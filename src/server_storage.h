/*
 * Copyright (C) 2014 Red Hat
 *
 * This file is part of openconnect-gui.
 *
 * openconnect-gui is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "keypair.h"

class StoredServer {
public:
    StoredServer();
    ~StoredServer();

    int load(QString& name);
    int save();

    const QString& get_username() const;
    void set_username(const QString& username);

    const QString& get_password() const;
    void set_password(const QString& password);

    const QString& get_groupname() const;
    void set_groupname(const QString& groupname);

    const QString& get_server_gateway() const;
    void set_server_gateway(const QString& server_gateway);

    const QString& get_label() const;
    void set_label(const QString& label);

    bool get_disable_udp() const;
    void set_disable_udp(bool v);

    QString get_cert_file();
    QString get_key_file();
    QString get_key_url() const;
    QString get_ca_cert_file();

    void clear_cert();
    void clear_key();
    void clear_ca();
    void clear_password();
    void clear_groupname();
    void clear_server_pin();

    QString get_client_cert_pin();
    int set_client_cert(const QString& filename);

    QString get_ca_cert_pin();
    int set_ca_cert(const QString& filename);

    bool get_batch_mode() const;
    void set_batch_mode(const bool mode);

    bool get_minimize() const;
    void set_minimize(const bool t);

    bool get_proxy() const;
    void set_proxy(const bool t);

    int get_reconnect_timeout() const;
    void set_reconnect_timeout(const int timeout);

    int get_dtls_reconnect_timeout() const;
    void set_dtls_reconnect_timeout(const int timeout);

    QString get_token_str();
    void set_token_str(const QString& str);

    int get_token_type();
    void set_token_type(const int type);

    const QString&  get_protocol_name() const;
    void set_protocol_name(const QString name);

    bool server_pin_algo_is_legacy(void);
    unsigned get_server_pin(QByteArray& hash) const;
    void get_server_pin(QString& hash) const;
    void set_server_pin(const unsigned algo, const QByteArray& hash);

    bool client_is_complete() const;

    int set_client_key(const QString& filename);

    QString m_last_err;

    const QString& get_interface_name() const;
    void set_interface_name(const QString& interface_name);

    const QString& get_vpnc_script_filename() const;
    void set_vpnc_script_filename(const QString& vpnc_script_filename);

    const QString& get_reported_os() const;
    void set_reported_os(const QString& reported_os);

    const QString& get_split_dns_domains() const;
    void set_split_dns_domains(const QString& domains);

    int get_log_level();
    void set_log_level(const int log_level);

    int get_icon_type() const;
    void set_icon_type(const int icon_type);

    bool get_ad_check_enabled() const { return m_ad_check_enabled; }
    void set_ad_check_enabled(bool v) { m_ad_check_enabled = v; }

    const QString& get_ad_domain() const { return m_ad_domain; }
    void set_ad_domain(const QString& d) { m_ad_domain = d; }

    const QString& get_ad_base_dn() const { return m_ad_base_dn; }
    void set_ad_base_dn(const QString& dn) { m_ad_base_dn = dn; }

    const QString& get_ad_srv_user() const { return m_ad_srv_user; }
    void set_ad_srv_user(const QString& u) { m_ad_srv_user = u; }

    int get_ad_user_expiry_days() const { return m_ad_user_expiry_days; }
    void set_ad_user_expiry_days(int d) { m_ad_user_expiry_days = d; }

    const QString& get_ad_user_expiry_text() const { return m_ad_user_expiry_text; }
    void set_ad_user_expiry_text(const QString& t) { m_ad_user_expiry_text = t; }

    int get_ad_srv_expiry_days() const { return m_ad_srv_expiry_days; }
    void set_ad_srv_expiry_days(int d) { m_ad_srv_expiry_days = d; }

    const QString& get_ad_srv_expiry_text() const { return m_ad_srv_expiry_text; }
    void set_ad_srv_expiry_text(const QString& t) { m_ad_srv_expiry_text = t; }

    const QString& get_ad_last_checked() const { return m_ad_last_checked; }
    void set_ad_last_checked(const QString& t) { m_ad_last_checked = t; }

    static void save_ad_cache(const QString& profileName, int userDays, const QString& userText, int srvDays, const QString& srvText);

private:
    bool m_batch_mode;
    bool m_minimize_on_connect;
    bool m_proxy;
    bool m_disable_udp;
    int m_reconnect_timeout;
    int m_dtls_attempt_period;
    QString m_username;
    QString m_password;
    QString m_groupname;
    QString m_server_gateway;
    QString m_token_string;
    QString m_label;
    int m_token_type;
    QString m_protocol_name;
    QByteArray m_server_pin;
    unsigned m_server_pin_algo;
    Cert m_ca_cert;
    KeyPair m_client;
    QString m_interface_name;
    QString m_vpnc_script_filename;
    QString m_reported_os;
    QString m_split_dns_domains;
    int m_log_level;
    int m_icon_type;

    bool m_ad_check_enabled;
    QString m_ad_domain;
    QString m_ad_base_dn;
    QString m_ad_srv_user;
    int m_ad_user_expiry_days;
    QString m_ad_user_expiry_text;
    int m_ad_srv_expiry_days;
    QString m_ad_srv_expiry_text;
    QString m_ad_last_checked;
};
