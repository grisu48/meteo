package org.feldspaten.meteo.meteo.meteo;

/**
 * Meteo station metadata
 */

public class Station {
    private long id;
    private String name;

    public Station() {
        id = 0L;
        name = "";
    }

    public Station(long id, String name) {
        this.id = id;
        this.name = name;
    }

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    @Override
    public String toString() {
        return name;
    }

    @Override
    public int hashCode() {
        return (int)id;
    }
}
