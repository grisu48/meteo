/** Collector instance */

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <sqlite3.h>
#include <pthread.h>

/** Station instance */
class Station {
private:
	bool aliveFlag = true;		// Mark station alive
	
	/** First dataset? */
	bool firstPush = true;
	
public:
	long id = 0L;
	std::string name = "";
	float t = 0.0F;
	float hum = 0.0F;
	float p = 0.0F;
	float l_ir = 0.0F;
	float l_vis = 0.0F;
	
	Station(const long id = 0L) {
		this->id = id;
		this->name = "";
		this->t = 0.0F;
		this->hum = 0.0F;
		this->p = 0.0F;
		this->l_ir = 0.0F;
		this->l_vis = 0.0F;
		this->aliveFlag = true;
		this->firstPush = true;
	}
	
	Station(const Station &src) {
		this->id = src.id;
		this->name = src.name;
		this->t = src.t;
		this->hum = src.hum;
		this->p = src.p;
		this->l_ir = src.l_ir;
		this->l_vis = src.l_vis;
		this->aliveFlag = src.aliveFlag;
		this->firstPush = src.firstPush;
	}
	
	/** Push a value set and mark station alive */
	void push(float t, float hum, float p, float l_ir, float l_vis, const float f_alpha = 0.9F) {
		if(this->firstPush) {
			this->firstPush = false;
			this->t = t;
			this->hum = hum;
			this->p = p;
			this->l_ir = l_ir;
			this->l_vis = l_vis;		
		} else {
			this->t = (f_alpha * this->t) + (1.0F-f_alpha) * t;
			this->hum = (f_alpha * this->hum) + (1.0F-f_alpha) * hum;
			this->p = (f_alpha * this->p) + (1.0F-f_alpha) * p;
			this->l_ir = (f_alpha * this->l_ir) + (1.0F-f_alpha) * l_ir;
			this->l_vis = (f_alpha * this->l_vis) + (1.0F-f_alpha) * l_vis;
		}
		this->aliveFlag = true;
	};
	
	/** @returns true if marked alive otherwise false */
	bool alive(void) const { return this->aliveFlag; }
	void alive(bool value) { this->aliveFlag = value; }
};

/** Datapoint for a meteo station */
class DataPoint {
public:
	long station = 0L;
	long timestamp = 0L;
	float t = 0.0F;
	float hum = 0.0F;
	float p = 0.0F;
	float l_ir = 0.0F;
	float l_vis = 0.0F;
	
	DataPoint() {
		this->station = 0L;
		this->timestamp = 0L;
		this->t = 0.0F;
		this->hum = 0.0F;
		this->p = 0.0F;
		this->l_ir = 0.0F;
		this->l_vis = 0.0F;
	}
	
	DataPoint(const DataPoint &src) {
		this->station = src.station;
		this->timestamp = src.timestamp;
		this->t = src.t;
		this->hum = src.hum;
		this->p = src.p;
		this->l_ir = src.l_ir;
		this->l_vis = src.l_vis;
	}
};

/** Lightning event */
class Lightning {
public:
	/** Timestamp in seconds since EPOC */
	long timestamp;
	/** Distance from station in km */
	float distance;
	/** Station id */
	long station;
	
	Lightning() {
		this->station = 0L;
		this->timestamp = 0L;
		this->distance = 0.0F;
	}
	
	Lightning(const Lightning &src) {
		this->station = src.station;
		this->timestamp = src.timestamp;
		this->distance = src.distance;
	}
	
	Lightning& operator=(const Lightning &src) {
		this->station = src.station;
		this->timestamp = src.timestamp;
		this->distance = src.distance;
		return *this;
	}
	
	std::string toString() const {
		std::stringstream ss;
		
		ss << "Lightning (" << this->timestamp << ") [Station " << this->station << "] " << this->distance << " km";
		return ss.str();
	}
};

/** Collects the data of different stations */
class Collector {
private:
	std::map<long, Station> stations;
	
	float f_alpha = 0.9F;
	int _delay = 5*60;		// Push Every 5 minutes by default
	
	volatile bool running = false;
	
	pthread_t tid;
	pthread_mutex_t mutex;
	
	sqlite3 *db = NULL;
	
	/** Verbosity level of the collector */
	int _verbose = 0;
	
protected:
	/** Purge the dead stations */
	void purge_stations();
	
	void mutex_lock() const;
	void mutex_unlock() const;
	
	void sql_exec(const std::string sql);
public:
	Collector();
	virtual ~Collector();
	
	void open_db(const char* filename);
	void close();
	
	void start();
	void run();
	
	int verbose(void) const { return this->_verbose; }
	void verbose(int verbose) { this->_verbose = verbose; }
	
	int delay() const { return this->_delay; }
	void delay(int delay) { this->_delay = delay; }
	
	/** Push a dataset */
	void push(long id, std::string name, float t, float hum, float p, float l_ir, float l_vis);
	/** Push a lightning event */
	void push(const Lightning &lightning);
	
	/** Get all currently active stations */
	std::vector<Station> activeStations(void) const;
	
	/** Returns the station with the given id or a new instance, if not existing */
	Station station(const long id) const;
	
	
	
	/** Query data from the database */
	std::vector<DataPoint> query(const long station, const long minTimestamp = -1L, const long maxTimestamp = -1L, const long limit = 1000, const long offset = 0L);
	
	std::vector<Lightning> query_lightnings(const long station, const long minTimestamp = -1L, const long maxTimestamp = -1L, const long limit = 1000, const long offset = 0L);
};
