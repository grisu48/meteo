package org.meteo;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;

/**
 * Meteo default TCP client instance
 * 
 * @author phoenix
 *
 */
public class Meteo {
	/** Callback when new data has been received */
	public interface RecvCallback {
		/**
		 * Received callback
		 * 
		 * @param data
		 *            Data that has been received
		 */
		public void onReceived(final MeteoData data);

		/**
		 * Called when an error occurred
		 * 
		 * @param e
		 *            that has been occurred
		 */
		public void onError(final IOException e);
	}

	/** Socket to communicate with host */
	private final Socket socket;
	/** Inputstream of the socket */
	private final InputStream in;
	/** {@link OutputStream} of the socekt */
	private final OutputStream out;
	/** Writer to the socket */
	private final PrintWriter writer;

	/** Receiver thread, if setup */
	private Thread recvThread = null;

	public Meteo(final String remoteHost, final int port)
			throws UnknownHostException, IOException {
		this.socket = new Socket(remoteHost, port);
		this.in = this.socket.getInputStream();
		this.out = this.socket.getOutputStream();
		this.writer = new PrintWriter(this.out);
	}

	@Override
	protected void finalize() throws Throwable {
		this.close();
		super.finalize();
	}

	/**
	 * Close ignore consequences
	 * 
	 * @param closeable
	 *            To be closed
	 */
	private void close(final Closeable closeable) {
		try {
			closeable.close();
		} catch (IOException e) {
		}
	}

	/**
	 * Close all connections to the host
	 */
	public synchronized void close() {
		close(writer);
		close(out);
		close(in);
		close(socket);

		if (recvThread != null) {
			recvThread.interrupt();
			recvThread = null;
		}
	}

	/**
	 * Reads a single char from the input stream
	 * 
	 * @return
	 */
	protected char readChar() throws IOException {
		final int iC = in.read();
		if (iC < 0)
			throw new IOException("Error reading from stream");
		return (char) iC;
	}

	protected String readLine() throws IOException {
		final StringBuffer buffer = new StringBuffer();

		int c;
		while (true) {
			c = in.read();
			if (c == 0)
				throw new IOException("Socket closed");
			else if (c < 0)
				throw new IOException("Receive error");
			else if (c == '\n')
				break;
			else
				buffer.append((char) c);
		}
		return buffer.toString();
	}

	static Map<String, String> parseLine(String line)
			throws IllegalArgumentException {

		// XXX Trivial XML parser. This is currently a ugly hack
		line = line.trim().toLowerCase();
		if (line.isEmpty())
			throw new IllegalArgumentException("Empty input");

		String data = "";
		if ((line.startsWith("<node ") && line.endsWith("/>"))) {
			data = line.substring(6, line.length() - 6 - 2).trim();
		} else if ((line.startsWith("<meteo ") && line.endsWith("/>"))) {
			data = line.substring(7, line.length() - 7 - 2).trim();
		} else {
			throw new IllegalArgumentException("Illegal format");
		}

		final Map<String, String> ret = new HashMap<String, String>();

		// Parse data
		boolean escaped = false;
		StringBuffer buffer = new StringBuffer();
		int state = 0;
		String name = "";
		String value = "";
		for (int i = 0; i < data.length(); i++) {
			final char c = data.charAt(i);

			if (escaped) {
				if (c == '\"') {
					escaped = false;
					// Process
				} else
					buffer.append(c);
			} else {
				if (c == '\"') {
					escaped = true;
					continue;
				} else if (c == '=') {
					if (state == 0) {
						name = buffer.toString().trim();
						buffer.delete(0, buffer.length());
						if (name.isEmpty())
							throw new IllegalArgumentException("Empty name");
						state = 1;
					} else if (state > 0)
						throw new IllegalArgumentException("Illegal format");
				} else if (Character.isWhitespace(c)) {
					if (state == 1) {
						// Done, next thingie
						value = buffer.toString().trim();
						buffer.delete(0, buffer.length());

						ret.put(name, value);
						state = 0;
					}
				} else
					buffer.append(c);
			}
		}

		// check state
		if (state == 1) {
			value = buffer.toString().trim();
			buffer.delete(0, buffer.length());
			if (!name.isEmpty())
				ret.put(name, value);
		}

		return ret;
	}

	private float parseFloat(final String str) {
		return parseFloat(str, 0.0F);
	}

	private float parseFloat(final String str, float defaultValue) {
		try {
			return Float.parseFloat(str.trim());
		} catch (NumberFormatException e) {
			return defaultValue;
		} catch (NullPointerException e) {
			return defaultValue;
		}
	}

	/**
	 * Receive a single {@link MeteoData} dataset
	 * 
	 * @return Next arriving {@link MeteoData} dataset
	 * @throws IOException
	 *             Thrown if occurring while reading from host
	 */
	public synchronized MeteoData receive() throws IOException {
		final String line = readLine();

		try {
			final Map<String, String> values = parseLine(line);

			final MeteoData data = new MeteoData();

			try {
				if (values.containsKey("id"))
					data.setStation(Long.parseLong(values.get("id")));
			} catch (NumberFormatException e) {
			}

			if (values.containsKey("name"))
				data.setStationName(values.get("name"));

			if (values.containsKey("humidity"))
				data.setHumidity(parseFloat(values.get("humidity")));
			if (values.containsKey("pressure"))
				data.setPressure(parseFloat(values.get("pressure")));
			if (values.containsKey("temperature"))
				data.setTemperature(parseFloat(values.get("temperature")));

			return data;

		} catch (NumberFormatException e) {
			throw new IOException("Illegal number format", e);
		} catch (IllegalArgumentException e) {
			throw new IOException("Illegal data", e);
		}
	}

	/**
	 * Start receiving in background thread
	 * 
	 * @param callback
	 *            callback that will be called when receiving data
	 */
	public synchronized void receive(final RecvCallback callback) {
		if (callback == null)
			throw new NullPointerException();
		if (recvThread != null)
			throw new IllegalStateException("Receiver thread already started");

		recvThread = new Thread() {
			@Override
			public void run() {
				while (Meteo.this.socket.isConnected() && !Thread.interrupted()) {
					try {
						final MeteoData data = Meteo.this.receive();
						callback.onReceived(data);
					} catch (IOException e) {
						callback.onError(e);
					}
				}
			};
		};
		recvThread.setDaemon(true);
		recvThread.start();

	}

	public static void main(final String[] args) throws UnknownHostException,
			IOException {
		int port = 8888;
		String remoteHost = "localhost";

		if (args.length >= 1)
			remoteHost = args[0];
		if (args.length >= 2) {
			try {
				port = Integer.parseInt(args[1]);
			} catch (NullPointerException e) {
				System.err.println("Illegal port number");
				System.exit(1);
			}
		}

		final Meteo meteo = new Meteo(remoteHost, port);
		System.out.println("Connected to " + remoteHost + ":" + port);
		meteo.receive(new RecvCallback() {

			@Override
			public void onReceived(MeteoData data) {
				System.out.println(data);
			}

			@Override
			public void onError(IOException e) {
				e.printStackTrace();
				System.exit(1);
			}
		});
		try {
			while (true) {
				Thread.sleep(10000L);
			}
		} catch (InterruptedException e) {

		}
	}
}
