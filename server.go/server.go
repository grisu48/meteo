/* Simple meteo data collection server, written in go
/* To thet it running, do
 *   go get github.com/mattn/go-sqlite3
 *   go get github.com/bmizerany/pat
 *   go run simple_REST.go
 */

package main

import (
	"database/sql"
	//"encoding/json"
	"fmt"
	"log"
	"net/http"
	//"strconv"

	"github.com/bmizerany/pat"
	_ "github.com/mattn/go-sqlite3"
)


/** That's the main database */
var mainDB *sql.DB



func main() {

	db, err := sql.Open("sqlite3", "meteo.db")
	if err != nil { panic(err) }
	initDb(db)
	mainDB = db

	r := pat.New()
	r.Get("/", http.HandlerFunc(getRoot))
	r.Get("/stations", http.HandlerFunc(getStations))
	r.Get("/station/:id", http.HandlerFunc(getStation))
	r.Post("/station/:id", http.HandlerFunc(postStation))
	/*
	r.Del("/todos/:id", http.HandlerFunc(deleteByID))
	r.Get("/todos/:id", http.HandlerFunc(getByID))
	r.Put("/todos/:id", http.HandlerFunc(updateByID))
	r.Get("/todos", http.HandlerFunc(getAll))
	r.Post("/todos", http.HandlerFunc(insert))
	*/

	http.Handle("/", r)
	log.Print(" meteo server running on http://localhost:12345/")
	err = http.ListenAndServe(":12345", nil)
	if err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}

func initDb(db *sql.DB) {
	var err error
	_, err = db.Exec("CREATE TABLE IF NOT EXISTS `stations` (`id` INTEGER PRIMARY KEY, `name` TEXT, `description` TEXT);")
	if err != nil {
		panic(err)
	}
}

func getRoot(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "meteo Server")
}


func getStations(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Stations request")
}


func getStation(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Request for station (?)")
}

func postStation(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Posting for station (?)")
}

/*

func getAll(w http.ResponseWriter, r *http.Request) {
	rows, err := mainDB.Query("SELECT * FROM todos")
	checkErr(err)
	var todos Todos
	for rows.Next() {
		var todo Todo
		err = rows.Scan(&todo.ID, &todo.Name)
		checkErr(err)
		todos = append(todos, todo)
	}
	jsonB, errMarshal := json.Marshal(todos)
	checkErr(errMarshal)
	fmt.Fprintf(w, "%s", string(jsonB))
}

func getByID(w http.ResponseWriter, r *http.Request) {
	id := r.URL.Query().Get(":id")
	stmt, err := mainDB.Prepare(" SELECT * FROM todos where id = ?")
	checkErr(err)
	rows, errQuery := stmt.Query(id)
	checkErr(errQuery)
	var todo Todo
	for rows.Next() {
		err = rows.Scan(&todo.ID, &todo.Name)
		checkErr(err)
	}
	jsonB, errMarshal := json.Marshal(todo)
	checkErr(errMarshal)
	fmt.Fprintf(w, "%s", string(jsonB))
}

func insert(w http.ResponseWriter, r *http.Request) {
	name := r.FormValue("name")
	var todo Todo
	todo.Name = name
	stmt, err := mainDB.Prepare("INSERT INTO todos(name) values (?)")
	checkErr(err)
	result, errExec := stmt.Exec(todo.Name)
	checkErr(errExec)
	newID, errLast := result.LastInsertId()
	checkErr(errLast)
	todo.ID = newID
	jsonB, errMarshal := json.Marshal(todo)
	checkErr(errMarshal)
	fmt.Fprintf(w, "%s", string(jsonB))
}

func updateByID(w http.ResponseWriter, r *http.Request) {
	name := r.FormValue("name")
	id := r.URL.Query().Get(":id")
	var todo Todo
	ID, _ := strconv.ParseInt(id, 10, 0)
	todo.ID = ID
	todo.Name = name
	stmt, err := mainDB.Prepare("UPDATE todos SET name = ? WHERE id = ?")
	checkErr(err)
	result, errExec := stmt.Exec(todo.Name, todo.ID)
	checkErr(errExec)
	rowAffected, errLast := result.RowsAffected()
	checkErr(errLast)
	if rowAffected > 0 {
		jsonB, errMarshal := json.Marshal(todo)
		checkErr(errMarshal)
		fmt.Fprintf(w, "%s", string(jsonB))
	} else {
		fmt.Fprintf(w, "{row_affected=%d}", rowAffected)
	}

}

func deleteByID(w http.ResponseWriter, r *http.Request) {
	id := r.URL.Query().Get(":id")
	stmt, err := mainDB.Prepare("DELETE FROM todos WHERE id = ?")
	checkErr(err)
	result, errExec := stmt.Exec(id)
	checkErr(errExec)
	rowAffected, errRow := result.RowsAffected()
	checkErr(errRow)
	fmt.Fprintf(w, "{row_affected=%d}", rowAffected)
}

func checkErr(err error) {
	if err != nil {
		panic(err)
	}
}
*/
