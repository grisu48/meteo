package org.meteo;

import java.io.Closeable;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.util.Map;

/**
 * Udp receiver instance
 * 
 * @author phoenix
 *
 */
public class UdpReceiver {
	/** Socket for receiving */
	private final DatagramSocket socket;

	/**
	 * Initialize new UDP receiver
	 * 
	 * @param port
	 *            Port where to listen on
	 * @throws SocketException
	 */
	public UdpReceiver(final int port) throws SocketException {
		this.socket = new DatagramSocket(port);
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

	public void close() {
		close(this.socket);
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
	 * Receive next {@link MeteoData} data
	 * 
	 * @return Next received data instance
	 * @throws IOException
	 *             Thrown if occurring while receiving
	 */
	public synchronized MeteoData receive() throws IOException {
		byte[] buf = new byte[1500];
		final DatagramPacket packet = new DatagramPacket(buf, buf.length);
		socket.receive(packet);
		final String strPacket = new String(packet.getData());

		try {
			Map<String, String> values = Meteo.parseLine(strPacket);

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

	public static void main(final String[] args) throws IOException {
		int port = 5232;

		final UdpReceiver receiver = new UdpReceiver(port);
		while (port == 5232) {
			System.out.println(receiver.receive());
		}
		receiver.close();
	}
}
