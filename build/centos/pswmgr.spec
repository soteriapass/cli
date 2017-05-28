Name:           pswmgr
Version:        0.1.4
Release:        1%{?dist}
Summary:        Password manager binaries

License:        GPL-3.0
URL:            https://github.com/viperman1271/%{name}
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc gcc-c++ make unzip curl libtool openssl openssl-devel

%description
Password Manager that uses a client/server architecture to store encrypted passwords remotely

%prep
%setup -q

%build
cd server
make %{?_smp_mflags}
cd ../src
make %{?_smp_mflags}
cd ../build/centos

%install
rm -rf $RPM_BUILD_ROOT
cd server
%make_install
cd ../src
%make_install

%pre
groupadd -rf pswmgr
id -u pswmgr &>/dev/null || useradd -r -d /etc/pswmgr/ -g pswmgr -s /sbin/nologin pswmgr

%post
chown pswmgr: /etc/%{name}/%{name}d.conf
chown pswmgr: /etc/%{name}/%{name}.conf
chown pswmgr: /usr/bin/%{name}d
chown pswmgr: /usr/bin/%{name}
chmod 600 /etc/%{name}/%{name}d.conf
chmod 600 /etc/%{name}/%{name}.conf

%files
%doc
/usr/bin/%{name}d
/etc/pswmgr/pswmgrd.conf
/usr/bin/%{name}
/etc/pswmgr/pswmgr.conf
/etc/init.d/pswmgr

%changelog
* Fri Feb 24 2017 Michael Filion <mfilion@mikefilion.com> 0.1.4-1
- Rebuilding after adding support to redirect grpc logging to common log stream
* Wed Feb 22 2017 Michael Filion <mfilion@mikefilion.com> 0.1.3-1
- Modified logging
- Added PID file
- Included service script
* Sun Feb 19 2017 Michael Filion <mfilion@mikefilion.com> 0.1.1-1
- Added client executable
* Mon Feb 13 2017 Michael Filion <mfilion@mikefilion.com> 0.1.0-1
- Initial package for CentOS
