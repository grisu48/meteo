package org.meteo;

/** Weather datapoint */
public class MeteoData {
	private long station;
	private String stationName;
	private int battery;
	private float humidity;
	private float temperature;
	private float pressure;
	private long timestamp;

	public MeteoData() {
		this.station = 0L;
		this.stationName = "";
		this.battery = 0;
		this.humidity = 0F;
		this.temperature = 0F;
		this.pressure = 0F;
		this.timestamp = System.currentTimeMillis();
	}

	public MeteoData(long station, String stationName, int battery,
			float humidity, float temperature, float pressure) {
		super();
		this.station = station;
		this.stationName = stationName;
		this.battery = battery;
		this.humidity = humidity;
		this.temperature = temperature;
		this.pressure = pressure;
		this.timestamp = System.currentTimeMillis();
	}

	public MeteoData(long station, int battery, float humidity,
			float temperature, float pressure) {
		super();
		this.station = station;
		this.stationName = "";
		this.battery = battery;
		this.humidity = humidity;
		this.temperature = temperature;
		this.pressure = pressure;
		this.timestamp = System.currentTimeMillis();
	}

	public long getStation() {
		return station;
	}

	public void setStation(long station) {
		this.station = station;
	}

	public String getStationName() {
		return stationName;
	}

	public void setStationName(String stationName) {
		this.stationName = stationName;
	}

	public int getBattery() {
		return battery;
	}

	public void setBattery(int battery) {
		this.battery = battery;
	}

	public float getHumidity() {
		return humidity;
	}

	public void setHumidity(float humidity) {
		this.humidity = humidity;
	}

	public float getTemperature() {
		return temperature;
	}

	public void setTemperature(float temperature) {
		this.temperature = temperature;
	}

	public float getPressure() {
		return pressure;
	}

	public void setPressure(float pressure) {
		this.pressure = pressure;
	}

	public long getTimestamp() {
		return timestamp;
	}

	public void setTimestamp(long timestamp) {
		this.timestamp = timestamp;
	}

	@Override
	public String toString() {
		StringBuffer ret = new StringBuffer();
		if (!(stationName == null || stationName.isEmpty()))
			ret.append(stationName + " ");
		ret.append("(" + station + ")");
		ret.append(" " + temperature + " deg C, " + humidity + " % rel "
				+ pressure + " hPa");

		return ret.toString();
	}
}
