/*
 * HostCtx.cxx
 *
 *  Created on: 2015年3月2日
 *      Author: Fifi Lyu
 */

#include "HostCtx.h"
#include "common.h"
#include <sstream>
#include <exception>
#include <fstream>
#include <iostream>

using std::cout;
using std::endl;
using std::exception;
using std::istringstream;
using std::ifstream;

HostCtx::KeyNameDescMap_t HostCtx::m_key_name_desc_map = HostCtx::crt_KeyNameDescMap();
HostCtx::DescKeyNameMap_t HostCtx::m_desc_key_name_map = HostCtx::crt_DescKeyNameMap();

HostCtx::HostCtx(const string &cfg_file) : m_cfg_file(cfg_file) {

}

HostCtx::~HostCtx() {
    delete_all_host_obj();
}

HostCtx::KeyNameDescMap_t HostCtx::crt_KeyNameDescMap() {
    KeyNameDescMap_t key_name_desc_map_;

    key_name_desc_map_[KN_Host] = "Host";
    key_name_desc_map_[KN_OS] = "OS";
    key_name_desc_map_[KN_IPAddress] = "IPAddress";
    key_name_desc_map_[KN_Port] = "Port";
    key_name_desc_map_[KN_UserName] = "UserName";
    key_name_desc_map_[KN_Password] = "Password";
    key_name_desc_map_[KN_Description] = "Description";

    return key_name_desc_map_;
}

HostCtx::DescKeyNameMap_t HostCtx::crt_DescKeyNameMap() {
    DescKeyNameMap_t desc_key_name_map_;

    desc_key_name_map_["Host"] = KN_Host;
    desc_key_name_map_["OS"] = KN_OS;
    desc_key_name_map_["IPAddress"] = KN_IPAddress;
    desc_key_name_map_["Port"] = KN_Port;
    desc_key_name_map_["UserName"] = KN_UserName;
    desc_key_name_map_["Password"] = KN_Password;
    desc_key_name_map_["Description"] = KN_Description;

    return desc_key_name_map_;
}

void HostCtx::validate_key(const string &s) {
    DescKeyNameMap_t::iterator it_ = m_desc_key_name_map.find(s);

    if (it_ == m_desc_key_name_map.end())
        throw common::MException(s + ": unknown directive");
}

HostCtx::KeyName_t HostCtx::to_keyname(const string &s) {
    validate_key(s);
    return m_desc_key_name_map[s];
}

void HostCtx::delete_all_host_obj() {
    for (HostPtrMap_t::iterator it_ = m_host_list.begin();
            it_ != m_host_list.end(); ++it_) {
        HostPtr_t host_ = it_->second;

        if (host_)
            delete host_;
    }
}

void HostCtx::parse_line(string &line) {
    static HostPtr_t host;
    string key_("");
    string value_("");

    line = common::trim(line);

    if (line.empty()) return;

    size_t pos_ = line.find_first_of(' ');

    if (pos_ == string::npos)
        throw common::MException(line + ": invalid directive");

    key_ = line.substr(0, pos_);
    KeyName_t kn_ = to_keyname(key_);
    value_ = line.substr(pos_ + 1);

    if (value_.empty())
        throw common::MException(line + ": invalid directive");

    if (kn_ == KN_Host) {
        host = new Host();
        const string id_(value_);
        m_host_list[id_] = host;
    }

    switch (kn_) {
        case KN_Host:
            host->setID(value_);
            break;
        case KN_OS:
            host->setOS(value_);
            break;
        case KN_IPAddress:
            host->setIPAddress(value_);
            break;
        case KN_Port:
            host->setPort(value_);
            break;
        case KN_UserName:
            host->setUserName(value_);
            break;
        case KN_Password:
            host->setPassword(value_);
            break;
        case KN_Description:
            host->setDescription(value_);
            break;
        default:
            break;
    }
}

void HostCtx::load() {
    HostPtr_t host_;
    string line_("");

    // 如果配置文件不存在，则建立空文件
    if (!common::is_file_exist(m_cfg_file))
        common::write_file(m_cfg_file, "");

    ifstream infile_(m_cfg_file.c_str(), ifstream::binary);

    if (!infile_)
        throw common::MException("cannot open \"" + m_cfg_file + "\".");

    while(getline(infile_, line_)) {
        try {
            parse_line(line_);
        } catch (common::MException &e) {
            infile_.close();
            throw common::MException(e.what());
        }
    }

    infile_.close();
}

string HostCtx::dump(HostPtr_t host) {
    string host_(
            "Host " + host->ID() + "\n"
            "    OS " + host->OS() + "\n"
            "    IPAddress " + host->IPAddress() + "\n"
            "    Port " + host->Port() + "\n"
            "    UserName " + host->UserName() + "\n"
            "    Password " + host->Password() + "\n"
            "    Description " + host->Description() + "\n"
    );

    return host_;
}

string HostCtx::dump() {
    string cfg_("");

    for (HostPtrMap_t::iterator it_ = m_host_list.begin();
            it_ != m_host_list.end(); ++it_) {
        // 最后一个
        if (it_ == --m_host_list.end())
            cfg_ += dump(it_->second);
        else
            cfg_ += dump(it_->second) + "\n";
    }

    return cfg_;
}

HostPtrMap_t& HostCtx::host_list() {
    return m_host_list;
}

HostPtr_t HostCtx::get_host(const string &id) {
    if (is_host_exist(id))
        return m_host_list[id];

    throw common::MException("Host not found.");
}

bool HostCtx::is_host_exist(const string &id) {
    return m_host_list.find(id) != m_host_list.end();
}

common::OS_t to_os(const string &os) {
    if (os == "linux")
        return common::OS_linux;
    else if (os == "windows")
        return common::OS_windows;
    else
        throw common::MException("Invalid os name.");
}

string to_default_port(common::OS_t os) {
    if (os == common::OS_linux)
        return "22";
    else if (os == common::OS_windows)
        return "3389";

    return "";
}

string to_default_usr(common::OS_t os) {
    if (os == common::OS_linux)
        return "root";
    else if (os == common::OS_windows)
        return "administrator";

    return "";
}

void HostCtx::add_host(HostPtr_t host) {
    common::OS_t os_ = to_os(host->OS());

    if (host->Port().empty())
        host->setPort(to_default_port(os_));

    if (host->UserName().empty())
        host->setUserName(to_default_usr(os_));

    if (host->Password().empty())
        host->setPassword(NONE_VALUE);

    if (host->Description().empty())
        host->setDescription(NONE_VALUE);

    validate_host(host);
    m_host_list[host->ID()] = host;
}

void HostCtx::del_host(const string &id) {
    HostPtrMap_t::iterator it_ = m_host_list.find(id);

    if (it_ != m_host_list.end())
        m_host_list.erase(it_);
}

void HostCtx::validate_host(HostPtr_t host) {
    if (!common::is_host_id_str(host->ID()))
        throw common::MException("Invalid host name.");

    if (host->OS().empty())
        throw common::MException("Invalid os.");

    if (host->IPAddress().empty())
        throw common::MException("Invalid ip address.");

    if (!common::is_port(host->Port()))
        throw common::MException("Invalid port.");

    if (host->Password().empty())
        throw common::MException("Invalid Password.");

    if (host->UserName().empty())
        throw common::MException("Invalid user name.");

    if (host->Description().empty())
        throw common::MException("Invalid description.");
}

void HostCtx::save() {
    const string cfg_(dump());
    common::write_file(m_cfg_file, cfg_);
}
