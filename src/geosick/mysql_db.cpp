#include <mysql++/dbdriver.h>
#include "geosick/mysql_db.hpp"

namespace geosick {

static int64_t read_i64(const mysqlpp::String& str) {
    const char* data = str.data();
    size_t size = str.size();

    int64_t num = 0;
    bool negative = false;

    size_t i = 0;
    if (size > 0 && data[i] == '-') {
        negative = true;
        ++i;
    }
    for (; i < size; ++i) {
        int64_t digit = int64_t(data[i] - '0');
        if (digit >= 10) {
            throw std::runtime_error("Invalid integer");
        }
        num = 10*num + digit;
    }
    if (negative) {
        num = -num;
    }
    return num;
}

static uint32_t read_u32(const mysqlpp::String& str) {
    return (uint32_t)read_i64(str);
}

static int32_t read_i32(const mysqlpp::String& str) {
    return (int32_t)read_i64(str);
}

static uint16_t read_u16(const mysqlpp::String& str) {
    return (uint16_t)read_i64(str);
}



namespace {
    using namespace mysqlpp;
    class SslModeOption final: public IntegerOption {
    public:
        explicit SslModeOption(unsigned mode): IntegerOption(mode) {}
        virtual Error set(DBDriver* dbd) override {
            return dbd->connected() ? Option::err_connected
                : !dbd->set_option(MYSQL_OPT_SSL_MODE, &arg_) ? Option::err_api_reject
                : Option::err_NONE;
        }
    };
}

MysqlDb::MysqlDb(const Config& cfg) {
    auto mode_str = cfg.mysql.ssl_mode;
    unsigned int mode_flag;
    if (mode_str == "DISABLED") {
        mode_flag = SSL_MODE_DISABLED;
    } else if (mode_str == "PREFERRED") {
        mode_flag = SSL_MODE_PREFERRED;
    } else if (mode_str == "REQUIRED") {
        mode_flag = SSL_MODE_REQUIRED;
    } else {
        throw std::runtime_error("Invalid value of mysql.ssl_mode: '" + mode_str + "'");
    }
    m_conn.driver()->set_option(new SslModeOption(mode_flag));

    m_conn.connect(cfg.mysql.db.c_str(), cfg.mysql.server.c_str(),
        cfg.mysql.user.c_str(), cfg.mysql.password.c_str());
}

std::unique_ptr<MysqlReader> MysqlDb::read_rows() {
    mysqlpp::Query query = m_conn.query(R"(
        SELECT client_id,
            UNIX_TIMESTAMP(created_at),
            TRUNCATE(CAST(lat AS DECIMAL(10,7))*10000000, 0),
            TRUNCATE(CAST(lng AS DECIMAL(10,7))*10000000, 0),
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
        uint32_t user_id = read_u32(row.at(0));
        uint32_t status = read_u32(row.at(1));
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
    res.user_id = read_u32(row.at(0));
    res.timestamp_utc_s = read_u32(row.at(1));
    res.lat = read_i32(row.at(2));
    res.lon = read_i32(row.at(3));
    res.accuracy_m = !row.at(4).is_null() ? read_u16(row.at(4)) : 50;
    if (!row.at(5).is_null()) {
        res.heading_deg = read_u16(row.at(5));
    }
    if (!row.at(6).is_null()) {
        res.velocity_mps = read_i32(row.at(6));
    }
    return res;
}

}
