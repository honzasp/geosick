#include "geosick/mysql_reader.hpp"

namespace geosick {

MysqlReader::MysqlReader(const std::string& db, const std::string& host, unsigned port,
    const std::string& user, const std::string& password)
{
    m_conn.connect(db.c_str(), host.c_str(), user.c_str(), password.c_str(), port);
    mysqlpp::Query query = m_conn.query(R"(
        SELECT client_id,
            UNIX_TIMESTAMP(created_at),
            CAST(lat AS DECIMAL(10,7))*10000000,
            CAST(lng AS DECIMAL(10,7))*10000000,
            accurancy,
            bear,
            speed
        FROM clients_positions
    )");
    m_result = query.use();
}

std::optional<GeoRow> MysqlReader::read() {
    auto row = m_result.fetch_row();
    if (!row) { return {}; }

    GeoRow res;
    res.user_id = row.at(0);
    res.timestamp_utc_s = row.at(1);
    res.lat = row.at(2);
    res.lon = row.at(3);
    res.accuracy_m = !row.at(4).is_null() ? row.at(4) : 50;
    if (!row.at(5).is_null()) {
        res.heading_deg = row.at(5);
    }
    if (!row.at(6).is_null()) {
        res.velocity_mps = row.at(6);
    }
    return res;
}

}
