#include <iostream>
#include <boost/algorithm/string/trim.hpp>
#include <string>
#include <regex>

using namespace std;


#include "zypp-plugin.h"


int
ZyppPlugin::main()
{
    while(true)
    {
	Message msg = read_message(pin);
	if (pin.eof())
	    break;
	Message reply = dispatch(msg);
	write_message(pout, reply);
    }
    return 0;
}


void
ZyppPlugin::write_message(ostream& os, const Message& msg)
{
    os << msg.command << endl;
    for(auto it: msg.headers) {
	os << it.first << ':' << it.second << endl;
    }
    os << endl;
    os << msg.body << '\0';
    os.flush();
}


ZyppPlugin::Message
ZyppPlugin::read_message(istream& is)
{
    enum class State {
		      Start,
		      Headers,
		      Body
    } state = State::Start;

    Message msg;

    while (!is.eof())
    {
	string line;

	getline(is, line);
	boost::trim_right(line);

	if (state == State::Start) {
	    if (is.eof())
		return msg; //empty

	    if (line.empty())
		continue;

	    static const regex rx_word("[A-Za-z0-9_]+", regex::extended);
	    if (regex_match(line, rx_word))
	    {
		msg = Message();
		msg.command = line;
		state = State::Headers;
	    }
	    else
	    {
		throw runtime_error("Plugin protocol error: expected a command. Got '" + line + "'");
	    }
	}
	else if (state == State::Headers) {
	    if (line.empty()) {
		state = State::Body;
		getline(is, msg.body, '\0');

		return msg;
	    }
	    else
	    {
		static const regex rx_header("([A-Za-z0-9_]+):[ \t]*(.+)", regex::extended);
		smatch match;

		if (regex_match(line, match, rx_header))
		{
		    string key = match[1];
		    string value = match[2];
		    msg.headers[key] = value;
		}
		else
		{
		    throw runtime_error("Plugin protocol error: expected a header or new line. Got '" + line + "'");
		}
	    }
	}
    }

    throw runtime_error("Plugin protocol error: expected a message, got a part of it");
}


ZyppPlugin::Message
ZyppPlugin::dispatch(const Message& msg)
{
    if (msg.command == "_DISCONNECT") {
	return ack();
    }
    Message a;
    a.command = "_ENOMETHOD";
    a.headers["Command"] = msg.command;
    return a;
}
