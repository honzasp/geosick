#include "geosick/mysql_reader.hpp"

namespace geosick {

MysqlReader::MysqlReader(const Config& cfg)
{
    m_conn.connect(cfg.mysql.db.c_str(), cfg.mysql.host.c_str(),
        cfg.mysql.user.c_str(), cfg.mysql.password.c_str(),
        cfg.mysql.port);
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
