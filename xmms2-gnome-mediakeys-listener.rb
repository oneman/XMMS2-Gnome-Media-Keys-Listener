#!/usr/bin/env ruby

# xmms2-gnome-mediakeys-listener.rb
# 
# this xmms2 client connects to the gnome settings daemon
# which broadcast out a dbus signal when those prev, play/pause, next keys are hit on your keyboard
# this listens for those signals and tells xmms2 to do something via simple command line
#
# 2010 rawdod / oneman / drr

require 'dbus'

bus = DBus::SessionBus.instance
service = bus.service("org.gnome.SettingsDaemon")
daemon = service.object("/org/gnome/SettingsDaemon/MediaKeys")

#puts service.introspect
daemon.introspect

daemon.default_iface = "org.gnome.SettingsDaemon.MediaKeys"

#puts service.introspect
#puts daemon.introspect

APPNAME = "Xmms2 listener"

daemon.GrabMediaPlayerKeys(APPNAME,0)

daemon.on_signal("MediaPlayerKeyPressed") do |app,key|
    # puts "Recived: #{app} #{key}"
    if app = APPNAME then
        case key
        when "Next"
          `nyxmms2 next`
        when "Previous"
          `nyxmms2 prev`
        when "Play"
          `nyxmms2 toggle`
        when "Stop"
          `nyxmms2 stop`
        when "Pause"
          `nyxmms2 pause`
        end
    end
end

#puts service.introspect
#puts daemon.introspect

puts "Its working."

loop = DBus::Main.new
loop << bus
loop.run
