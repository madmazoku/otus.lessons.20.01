#define BOOST_TEST_MODULE TestMain
#include <boost/test/unit_test.hpp>

#include "../bin/version.h"

#include "processor.h"

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_version )
{
    BOOST_CHECK_GT(build_version(), 0);
}

class ProcessorMock : public Processor
{
public:
    bool _process_called;
    std::vector<Commands> _commands;

    virtual void process(const Commands& commands) final {
        _process_called = true;
        _commands.push_back(commands);
    }

    ProcessorMock() : _process_called(false) {}
};

BOOST_AUTO_TEST_CASE( test_subscriber )
{
    Commands commands;
    commands.push_back(std::make_tuple(1, "1"));
    commands.push_back(std::make_tuple(2, "2"));
    commands.push_back(std::make_tuple(3, "3"));

    ProcessorSubscriber ps;
    ProcessorMock pm;

    ps.subscribe(&pm);

    ps.process(commands);

    BOOST_CHECK(pm._process_called);

    BOOST_CHECK(pm._commands.size() == 1);
    BOOST_CHECK(pm._commands[0].size() == commands.size());

    BOOST_CHECK(pm._commands[0] ==
    (Commands{
        std::make_tuple(1, "1"), std::make_tuple(2, "2"), std::make_tuple(3, "3")
    }));
}

BOOST_AUTO_TEST_CASE( test_console_print )
{
    Commands commands;
    commands.push_back(std::make_tuple(1, "1"));
    commands.push_back(std::make_tuple(2, "2"));
    commands.push_back(std::make_tuple(3, "3"));

    ProcessorSubscriber ps;
    std::stringstream ss;
    ConsolePrint cp(ps, ss);

    ps.process(commands);

    BOOST_CHECK(ss.str() == "bulk: 1, 2, 3\n");
}

class FileWriterMock : public IFileWriter
{
public:
    std::stringstream _out;
    std::string _name;
    bool _open_called;
    bool _close_called;

    virtual void open(const std::string& name) final {
        _name = name;
        _open_called = true;
    }
    virtual std::ostream& out() final {
        return _out;
    }
    virtual void close() final {
        _close_called = true;
    }

    FileWriterMock() : _open_called(false), _close_called(false) {}
};

BOOST_AUTO_TEST_CASE( test_file_print )
{
    Commands commands;
    commands.push_back(std::make_tuple(1, "1"));
    commands.push_back(std::make_tuple(2, "2"));
    commands.push_back(std::make_tuple(3, "3"));

    ProcessorSubscriber ps;
    std::stringstream ss;
    FileWriterMock* fwm = new FileWriterMock;
    FilePrint fp(ps, fwm);

    ps.process(commands);

    BOOST_CHECK(fwm->_open_called);
    BOOST_CHECK(fwm->_name == "bulk1.log");
    BOOST_CHECK(fwm->_out.str() == "1\n2\n3\n");
    BOOST_CHECK(fwm->_close_called);
}

class TimeMock : public ITime
{
public:
    size_t _count;
    virtual time_t get() final {
        return ++_count;
    };

    TimeMock() : _count(0) {}
};

BOOST_AUTO_TEST_CASE( test_reader_0_plain )
{
    TimeMock* tm = new TimeMock;
    std::stringstream ss;
    Reader r(0, tm);
    ProcessorMock pm;

    r.subscribe(&pm);

    ss << "1" << std::endl
       << "2" << std::endl
       << "3" << std::endl;
    r.read(ss);

    BOOST_CHECK(pm._commands.size() == 1);
    BOOST_CHECK(pm._commands[0].size() == 3);

    BOOST_CHECK(pm._commands[0] ==
    (Commands{
        std::make_tuple(1, "1"), std::make_tuple(2, "2"), std::make_tuple(3, "3")
    }));
}

BOOST_AUTO_TEST_CASE( test_reader_2_plain )
{
    TimeMock* tm = new TimeMock;
    std::stringstream ss;
    Reader r(2, tm);
    ProcessorMock pm;

    r.subscribe(&pm);

    ss << "1" << std::endl
       << "2" << std::endl
       << "3" << std::endl;
    r.read(ss);

    BOOST_CHECK(pm._commands.size() == 2);
    BOOST_CHECK(pm._commands[0].size() == 2);
    BOOST_CHECK(pm._commands[1].size() == 1);

    BOOST_CHECK(pm._commands[0] ==
    (Commands{
        std::make_tuple(1, "1"), std::make_tuple(2, "2")
    }));
    BOOST_CHECK(pm._commands[1] ==
    (Commands{
        std::make_tuple(3, "3")
    }));
}

BOOST_AUTO_TEST_CASE( test_reader_2_bracket_1 )
{
    TimeMock* tm = new TimeMock;
    std::stringstream ss;
    Reader r(2, tm);
    ProcessorMock pm;

    r.subscribe(&pm);

    ss << "1" << std::endl
       << "{" << std::endl
       << "2" << std::endl
       << "3" << std::endl
       << "4" << std::endl
       << "}" << std::endl;
    r.read(ss);

    BOOST_CHECK(pm._commands.size() == 2);
    BOOST_CHECK(pm._commands[0].size() == 1);
    BOOST_CHECK(pm._commands[1].size() == 3);

    BOOST_CHECK(pm._commands[0] ==
    (Commands{
        std::make_tuple(1, "1")
    }));
    BOOST_CHECK(pm._commands[1] ==
    (Commands{
        std::make_tuple(2, "2"), std::make_tuple(3, "3"), std::make_tuple(4, "4")
    }));
}

BOOST_AUTO_TEST_CASE( test_reader_2_bracket_2 )
{
    TimeMock* tm = new TimeMock;
    std::stringstream ss;
    Reader r(2, tm);
    ProcessorMock pm;

    r.subscribe(&pm);

    ss << "1" << std::endl
       << "{" << std::endl
       << "2" << std::endl
       << "{" << std::endl
       << "3" << std::endl
       << "}" << std::endl
       << "4" << std::endl
       << "}" << std::endl;
    r.read(ss);

    BOOST_CHECK(pm._commands.size() == 2);
    BOOST_CHECK(pm._commands[0].size() == 1);
    BOOST_CHECK(pm._commands[1].size() == 3);

    BOOST_CHECK(pm._commands[0] ==
    (Commands{
        std::make_tuple(1, "1")
    }));
    BOOST_CHECK(pm._commands[1] ==
    (Commands{
        std::make_tuple(2, "2"), std::make_tuple(3, "3"), std::make_tuple(4, "4")
    }));
}

BOOST_AUTO_TEST_CASE( test_reader_2_bracket_1_not_closed )
{
    TimeMock* tm = new TimeMock;
    std::stringstream ss;
    Reader r(2, tm);
    ProcessorMock pm;

    r.subscribe(&pm);

    ss << "1" << std::endl
       << "{" << std::endl
       << "2" << std::endl
       << "3" << std::endl
       << "4" << std::endl;
    r.read(ss);

    BOOST_CHECK(pm._commands.size() == 1);
    BOOST_CHECK(pm._commands[0].size() == 1);

    BOOST_CHECK(pm._commands[0] ==
    (Commands{
        std::make_tuple(1, "1")
    }));
}

BOOST_AUTO_TEST_SUITE_END()

