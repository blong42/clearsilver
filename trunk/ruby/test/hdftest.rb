#!/usr/bin/env ruby

require 'neo'

h=Neo::Hdf.new
h.set_value "1","farming"
h.set_value "2","sewing"
h.set_value "3","bowling"

h.set_value "party.1","baloons"
h.set_value "party.2","noise makers"
h.set_value "party.3","telling long\nstories"

h.set_attr "party.1", "Drool", "True"
h.set_attr "party.2", "Pink", "1"

print h.dump

q=Neo::Hdf.new

q.copy "arf",h

print q.dump

h.get_attr("party.2").each_pair do |k,v|
  print "party.2 attr (#{k}=#{v})\n"
end


s="This is a funny test. <?cs var:arf.1 ?>.
<?cs each:p = arf.party ?>
<?cs var:p ?>
<?cs /each ?>"
c = Neo::Cs.new q

c.parse_string(s)

print c.render

