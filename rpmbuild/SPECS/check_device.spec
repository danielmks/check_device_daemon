Name:           check_device
Version:        1.9
Release:        1%{?dist}
Summary:        Device monitoring daemon that logs date, time, CPU and memory usage every INTERVAL_MINUTES minutes

License:        GPL
Group:          System Environment/Base
URL:            http://example.com
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc, make, net-snmp-devel
Requires:  net-snmp

%global debug_package %{nil}

%description
check_device is a daemon that logs the current date, time, CPU usage, and memory usage every INTERVAL_MINUTES minutes.
Logs are stored in /var/log/check_device as "check_device_log_YYYYMMDD.log", and files older than 7 days are removed.

%prep
%setup -q

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

%post
# Ensure log directory exists on the target system with correct permissions
if [ ! -d /var/log/check_device ]; then
    mkdir -p /var/log/check_device
fi
chown nobody:nobody /var/log/check_device
chmod 0755 /var/log/check_device

%files
%defattr(-,root,root,-)
# Binary file
/usr/local/bin/check_device
# Systemd service file
/etc/systemd/system/check_device.service
# Log directory
%dir %attr(0755,nobody,nobody) /var/log/check_device
# Configure file
%config(noreplace) /etc/check_device/check_device.conf

%changelog
* Thu Mar 13 2025 keixin001 <keixin001@gmail.com> - 1.0-1
- Initial package for check_device daemon.

