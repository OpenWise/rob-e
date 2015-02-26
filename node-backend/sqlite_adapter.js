var sqlite3 = require('sqlite3').verbose();
var console;

//TODO: protect against SQL injection (escape user input)

function SqliteAdapter(dbFile) {
    var self = this;
    this.db = new sqlite3.Database(dbFile);

    var sql = this.db;
    sql.serialize(function() {
        sql.run("CREATE TABLE IF NOT EXISTS `robe_history` (" +
            "`record_id`    UNSIGNEDBIGINT NOT NULL PRIMARY KEY," +
            "`coor_x`       INTEGER NOT NULL," +
            "`coor_y`       INTEGER NOT NULL," +
            "`coor_z`       INTEGER NOT NULL," +
            "`coor_p`       INTEGER NOT NULL);");
    });
}

SqliteAdapter.prototype.ArchiveRobeMotion = function(coordinates) {
    var sql = this.db;
    sql.serialize(function() {
        sql.run("INSERT INTO `robe_history` (`record_id`, `coor_x`, `coor_y`, `coor_z`, `coor_p`) " +
            "VALUES (NULL," + coordinates.x + "," + coordinates.y + "," + coordinates.z +  "," + coordinates.p + ");");
    });
}

function SqliteFactory(c) {
    console = c;
    return SqliteAdapter;
}

module.exports = SqliteFactory;
