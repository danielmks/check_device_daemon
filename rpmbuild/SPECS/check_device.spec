Name:           check_device
Version:        3.0.1
Release:        1%{?dist}
Summary:        Device monitoring daemon that logs date, time, CPU and memory usage every periods

License:        GPL
Group:          System Environment/Base
URL:            http://example.com
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc, make, net-snmp-devel
Requires:  net-snmp

Provides:       libaxio.so.0()(64bit)

%global debug_package %{nil}

%description
check_device is a daemon that logs the current date, time, CPU usage, and memory usage every periods

%prep
%setup -q -n check_device

%build
make

%install
rm -rf %{buildroot}
# Install binary
mkdir -p %{buildroot}/usr/local/bin
install -m 0755 check_device %{buildroot}/usr/local/bin/check_device

# Install systemd service file
mkdir -p %{buildroot}/etc/systemd/system
install -m 0644 check_device.service %{buildroot}/etc/systemd/system/check_device.service

# Install configure file 
mkdir -p %{buildroot}/etc/check_device
install -m 0644 check_device.conf %{buildroot}/etc/check_device/check_device.conf

# Create log directory with proper permissions
mkdir -p %{buildroot}/var/log/check_device
chmod 0755 %{buildroot}/var/log/check_device

# Install rsyslog configuration file to /etc/rsyslog.d
mkdir -p %{buildroot}/etc/rsyslog.d
install -m 0644 check_device_rsyslog.conf %{buildroot}/etc/rsyslog.d/check_device_rsyslog.conf

# Install shared library to /usr/lib64
mkdir -p %{buildroot}/usr/lib64
install -m 0755 LIB/libaxio.so.1.0.0 %{buildroot}/usr/lib64/libaxio.so.1.0.0
# 심볼릭 링크 생성
cd %{buildroot}/usr/lib64 && ln -sf libaxio.so.1.0.0 libaxio.so.0
cd %{buildroot}/usr/lib64 && ln -sf libaxio.so.1.0.0 libaxio.so

%post
# Ensure log directory exists on the target system with correct permissions
if [ ! -d /var/log/check_device ]; then
    mkdir -p /var/log/check_device
fi
chown root:root /var/log/check_device
chmod 0755 /var/log/check_device

%files
%defattr(-,root,root,-)
# Binary file
/usr/local/bin/check_device
# Systemd service file
/etc/systemd/system/check_device.service
# Log directory
%dir %attr(0755,root,root) /var/log/check_device
# Configure file
%config(noreplace) /etc/check_device/check_device.conf
# Rsyslog configuration file
%config(noreplace) /etc/rsyslog.d/check_device_rsyslog.conf
# libaxio 라이브러리 및 심볼릭 링크들
/usr/lib64/libaxio.so.1.0.0
/usr/lib64/libaxio.so.0
/usr/lib64/libaxio.so

%changelog
* Thu Mar 13 2025 keixin001 <keixin001@gmail.com> - 1.0-1
- Initial package for check_device daemon.

