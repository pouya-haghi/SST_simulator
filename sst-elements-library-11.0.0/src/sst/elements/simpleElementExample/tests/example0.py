import sst

# In Example0, two components send each other a number of events 
# The simulation ends when the components have sent and
# received all expected events. While the eventSize is parameterized,
# it has no effect on simulation time because the components don't limit
# their link bandwidth
#
# Relevant code:
#   simpleElementExample/example0.h
#   simpleElementExample/example0.cc
#   simpleElementExample/basicEvent.h
# Output:
#   simpleElementExample/tests/refFiles/example0.out
#

### Create the components
component0 = sst.Component("2", "simpleElementExample.example0")
component1 = sst.Component("3", "simpleElementExample.example0")
component2 = sst.Component("4", "simpleElementExample.example0")
component3 = sst.Component("5", "simpleElementExample.example0")

### Parameterize the components.
# Run 'sst-info simpleElementExample.example0' at the command line 
# to see parameter documentation
params = {
        "eventsToSend" : 50,    # Required parameter, error if not provided
        "eventSize" : 32        # Optional parameter, defaults to 16 if not provided
}
component0.addParams(params)
component1.addParams(params)
component2.addParams(params)
component3.addParams(params)

# Link the components via their 'port' ports
link1 = sst.Link("component_link1")
link1.connect( (component0, "port1", "1ns"), (component1, "port1", "1ns") )
link2 = sst.Link("component_link2")
link2.connect( (component0, "port0", "1ns"), (component2, "port0", "1ns") )
link3 = sst.Link("component_link3")
link3.connect( (component1, "port0", "1ns"), (component3, "port0", "1ns") )
link4 = sst.Link("component_link4")
link4.connect( (component2, "port1", "1ns"), (component3, "port1", "1ns") )
