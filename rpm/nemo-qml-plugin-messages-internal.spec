# 
# Do NOT Edit the Auto-generated Part!
# Generated by: spectacle version 0.27
# 

Name:       nemo-qml-plugin-messages-internal

# >> macros
# << macros

Summary:    QML plugin for internal messages functionality
Version:    0.1.1
Release:    1
Group:      System/Libraries
License:    BSD
URL:        https://github.com/nemomobile/nemo-qml-plugin-messages
Source0:    %{name}-%{version}.tar.bz2
Source100:  nemo-qml-plugin-messages-internal.yaml
BuildRequires:  pkgconfig(QtCore) >= 4.7.0
BuildRequires:  pkgconfig(QtDeclarative)
BuildRequires:  pkgconfig(QtGui)
BuildRequires:  pkgconfig(TelepathyQt4)

%description
QML plugin for internal messages functionality in Nemo

%prep
%setup -q -n %{name}-%{version}

# >> setup
# << setup

%build
# >> build pre
# << build pre

%qmake 

make %{?jobs:-j%jobs}

# >> build post
# << build post

%install
rm -rf %{buildroot}
# >> install pre
# << install pre
%qmake_install

# >> install post
# << install post

%files
%defattr(-,root,root,-)
%{_libdir}/qt4/imports/org/nemomobile/messages/internal/*
# >> files
# << files
