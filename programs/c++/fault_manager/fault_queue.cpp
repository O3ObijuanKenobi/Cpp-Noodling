#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <sstream>

#include <boost/fusion/adapted/struct.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/phoenix/phoenix.hpp>

#include "fault_queue.hpp"
#include "fault_manager_constants.hpp"
#include "fault_struct.hpp"

int str2int (const std::string &str)
{
    int number;
    std::istringstream iss (str);
    iss >> number;
    if (iss.fail())
    {
        // TODO: add in conversion failure notifier
    }
    return number;
}

// Pass queue by value so that original queue isn't modified
template<typename T> void print_queue(T q)
{
    std::cout << "Queue size: " << q.size() << std::endl;
    while(!q.empty())
    {
        q.top().print();
        q.pop();
    }
    std::cout << '\n';
}

// Sorting rule that is applied when inserting new items into the priority queue
auto sortRuleLambda = [] (fault_event_t const& s1, fault_event_t const& s2) -> bool
{
    return s1 > s2;
};

std::priority_queue<fault_event_t, std::vector<fault_event_t>, decltype(sortRuleLambda)> priority_queue_instance(sortRuleLambda);

fault_queue::fault_queue()
{
    read_buffer();
}

fault_queue::~fault_queue()
{
    write_buffer();
}

void fault_queue::add_event(int _time_stamp, std::string _fault_message)
{
    fault_event_t new_event = {_time_stamp, _fault_message};
    priority_queue_instance.push(new_event);

    while (get_queue_length() > FM_CONSTANTS::MAX_BUFFER_SIZE)
    {
        priority_queue_instance.pop();
    }

    new_events_to_save = true;
}

void fault_queue::add_event(std::string _time_stamp, std::string _fault_message)
{
    add_event(str2int(_time_stamp), _fault_message);
}

int fault_queue::get_queue_length()
{
    return priority_queue_instance.size();
}

void fault_queue::print_buffer()
{
    print_queue(priority_queue_instance);
}

void fault_queue::read_buffer()
{
    std::string line;
    std::ifstream fault_file(FM_CONSTANTS::DATA_STORAGE_FILE);
    if (fault_file.is_open())
    {
        while ( getline (fault_file, line) )
        {
            size_t pos = 0;
            std::string token;
            while ((pos = line.find(FM_CONSTANTS::DELIMITER)) != std::string::npos)
            {
                token = line.substr(0, pos);
                //std::cout << token << std::endl;
                line.erase(0, pos + FM_CONSTANTS::DELIMITER.length());
            }
            //std::cout << line << std::endl;

            add_event(token, line);
        }
        fault_file.close();
    }
    else
    {
        std::cout << "Unable to open file\n";
    }
    std::cout << std::endl;
}


// Apply boost magic to allow structure to be interpreted during for_each/arg_name loop
BOOST_FUSION_ADAPT_STRUCT(fault_event_t, (int, time_stamp) (std::string, event_string));

void fault_queue::write_buffer()
{
    if (new_events_to_save)
    {
        std::ofstream fault_file;
        fault_file.open(FM_CONSTANTS::DATA_STORAGE_FILE);

        // Loop through all events to be written to file
        while(!priority_queue_instance.empty())
        {
            std::stringstream temp_event_line;
            // Iterate over all members of the struct, appending each to stringstream
            boost::fusion::for_each(priority_queue_instance.top(), temp_event_line << boost::phoenix::arg_names::arg1 << FM_CONSTANTS::DELIMITER);
            // Remove trailing ':' by seeking back one character and writing a new line char
            temp_event_line.seekp(-1, std::ios_base::end);
            temp_event_line << "\n";
            // Write stringstream to file
            fault_file << temp_event_line.rdbuf();
            priority_queue_instance.pop();
        }
        fault_file.close();

        new_events_to_save = false;
    }
}