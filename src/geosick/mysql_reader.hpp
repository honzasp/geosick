#pragma once
#include <mysql++/mysql++.h>
#include "geosick/config.hpp"
#include "geosick/geo_row_reader.hpp"

namespace geosick {

class MysqlReader final: public GeoRowReader {
    mysqlpp::Connection m_conn;
    mysqlpp::UseQueryResult m_result;
public:
    MysqlReader(const Config& cfg);
    virtual std::optional<GeoRow> read() override;
};

}
