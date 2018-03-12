#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <tuple>
#include <map>
#include <memory>

#include <boost/lexical_cast.hpp>

#include "queue_processor.h"
#include "metrics.h"

// Travis do not have it
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

using Command = std::tuple<time_t, std::string>;
using Commands = std::vector<Command>;

std::ostream& operator<<(std::ostream& out, const Command& command)
{
    out << " {" << std::get<0>(command) << ", " << std::get<1>(command) << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const Commands& commands)
{
    for(auto c : commands)
        std::cerr << ' ' << c;
    return out;
}

class Processor : public QueueProcessor<Commands>
{
public:

    Processor(size_t thread_count) : QueueProcessor<Commands>()
    {
        start(thread_count);
    }

    void process(const Commands& commands)
    {
        add(commands, true);
    }

    void done()
    {
        stop();
        wait();
        finish();
    }
};
using Processors = std::vector<Processor*>;

class ProcessorSubscriber
{
private:
    Processors _processors;

public:

    void process(const Commands& commands)
    {
        for(auto p : _processors)
            p->process(commands);
    }

    void subscribe(Processor* processor)
    {
        _processors.push_back(processor);
    }
};

class ConsolePrint : public Processor
{
private:
    std::ostream& _out;

    virtual void act(const Commands& commands, size_t qid) final {
        if(commands.size() == 0)
            return;

        Metrics::get_metrics().update("console.blocks", 1);
        Metrics::get_metrics().update("console.commands", commands.size());

        _out << "bulk: ";
        bool first = true;
        for(auto c :commands)
        {
            if(first)
                first = false;
            else
                _out << ", ";
            _out << std::get<1>(c);
        }
        _out << std::endl;
    }

public:
    ConsolePrint(ProcessorSubscriber& ps, std::ostream& out = std::cout) : Processor(1), _out(out)
    {
        ps.subscribe(this);
    }
};

class IFileWriter
{
public:
    using IFileWriterPtr = std::unique_ptr<IFileWriter>;

    virtual void open(const std::string& name) = 0;
    virtual std::ostream& out() = 0;
    virtual void close() = 0;
    virtual IFileWriterPtr clone() = 0;

    virtual ~IFileWriter() = default;
};
using IFileWriterPtr = IFileWriter::IFileWriterPtr;

class FileWriter : public IFileWriter
{
private:
    std::ofstream _out;

public:
    virtual void open(const std::string& name) final {
        _out.open(name, std::ios::app);
    }
    virtual std::ostream& out() final {
        return _out;
    }
    virtual void close() final {
        _out.close();
    }
    virtual IFileWriterPtr clone() final {
        return make_unique<FileWriter>();
    }
};

class FilePrint : public Processor
{
private:
    IFileWriterPtr _file_writer;
    std::map<time_t, size_t> _log_counter;
    std::mutex _log_counter_mutex;

    virtual void act(const Commands& commands, size_t qid) final {
        if(commands.size() == 0)
            return;

        std::string prefix = "file.";
        Metrics::get_metrics().update(prefix + "blocks");
        Metrics::get_metrics().update(prefix + "commands", commands.size());
        prefix += boost::lexical_cast<std::string>(qid);
        Metrics::get_metrics().update(prefix + ".blocks");
        Metrics::get_metrics().update(prefix + ".commands", commands.size());

        std::string name = "bulk";
        time_t tm = std::get<0>(*(commands.begin()));
        name += boost::lexical_cast<std::string>(tm);
        name += "-";

        size_t log_counter = 0;
        {
            std::lock_guard<std::mutex> lock_file(_log_counter_mutex);
            auto it_cnt = _log_counter.find(tm);
            if(it_cnt != _log_counter.end())
                log_counter = ++(it_cnt->second);
            else
                _log_counter[tm] = log_counter;
        }
        std::string cnt = boost::lexical_cast<std::string>(log_counter);

        IFileWriterPtr file_writer(_file_writer->clone());
        file_writer->open(name + cnt + ".log");
        for(auto c :commands)
            file_writer->out() << std::get<1>(c) << std::endl;
        file_writer->close();
    }

public:
    FilePrint(ProcessorSubscriber& ps, IFileWriterPtr file_writer = make_unique<FileWriter>()) : Processor(2), _file_writer(std::move(file_writer))
    {
        ps.subscribe(this);
    }
};

class ITime
{
public:
    virtual time_t get() = 0;

    virtual ~ITime() = default;
};
using ITimePtr = std::unique_ptr<ITime>;

class Time : public ITime
{
public:
    virtual time_t get() final {
        return std::time(nullptr);
    };
};

class Reader : public ProcessorSubscriber
{
private:
    ITimePtr _time;

    size_t _N;

    void flush(Commands& commands)
    {
        if(commands.size() > 0) {
            Metrics::get_metrics().update("reader.blocks");
            Metrics::get_metrics().update("reader.commands", commands.size());

            process(commands);
            commands.clear();
        }
    }

public:
    Reader(size_t N = 0, ITimePtr time = make_unique<Time>()) : _time(std::move(time)), _N(N) {}

    void read(std::istream& in)
    {
        Commands commands;
        size_t bracket_counter = 0;
        std::string line;
        while(std::getline(in, line)) {
            Metrics::get_metrics().update("reader.lines");
            if(line == "{") {
                if(bracket_counter++ == 0)
                    flush(commands);
            } else if(line == "}") {
                if(bracket_counter > 0 && --bracket_counter == 0)
                    flush(commands);
            } else {
                commands.push_back(std::make_tuple(_time->get(), line));
                if(bracket_counter == 0 && _N != 0 && commands.size() == _N)
                    flush(commands);
            }
        }
        if(bracket_counter == 0)
            flush(commands);
    }
};
