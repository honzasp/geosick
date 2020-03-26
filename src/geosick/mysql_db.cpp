#include "geosick/mysql_db.hpp"

namespace geosick {

MysqlDb::MysqlDb(const Config& cfg) {
    m_conn.connect(cfg.mysql.db.c_str(), cfg.mysql.host.c_str(),
        cfg.mysql.user.c_str(), cfg.mysql.password.c_str(),
        cfg.mysql.port);
}

std::unique_ptr<MysqlReader> MysqlDb::read_rows() {
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
    return std::make_unique<MysqlReader>(query.use());
}

MysqlDb::UserIds MysqlDb::read_user_ids() {
    mysqlpp::Query query = m_conn.query(R"(
        SELECT client_id, status
        FROM clients_statuses
    )");
    auto result = query.use();

    MysqlDb::UserIds user_ids;
    while (auto row = result.fetch_row()) {
        uint32_t user_id = row.at(0);
        uint32_t status = row.at(1);
        if (status == 1) {
            user_ids.sick.insert(user_id);
        } else if (status == 0) {
            user_ids.query.insert(user_id);
        }
    }
    return user_ids;
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
