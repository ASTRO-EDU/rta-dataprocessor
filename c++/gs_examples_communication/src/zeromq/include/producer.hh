#ifndef PRODUCER_HH
#define PRODUCER_HH

#include <zmq.hpp>
#include <iostream>

#include "../../../include/utils.hh"

// General producer template
template <typename T>
class Producer{
public:
    Producer(zmq::context_t &context, const std::string &address)
    : socket(context, ZMQ_PUSH){
        socket.connect(address);
    }

    // Destructor to clean up the socket
    virtual ~Producer(){
        try{
            socket.close();
        }
        catch (const zmq::error_t &e){
            std::cerr << "Error closing socket: " << e.what() << std::endl;
        }
    }

    void produce(const T& value){
        sendMessage(socket, [value]() -> zmq::message_t{
            // Create a message with the size of T
            // zmq::message_t message(sizeof(T));
            zmq::message_t message(value.size());

            // Copy the value into the message data
            // memcpy(message.data(), &value, sizeof(T));
            memcpy(message.data(), value.data(), value.size());

            //std::cout << "msg size is: " << message.size() << std::endl;
            
	        // Return the message
            return message;
        });
    }

    private:
        zmq::socket_t socket;
};

// Specialization for std::vector<T>
template <typename T>
class Producer<std::vector<T>>{
    public:
    Producer(zmq::context_t &context, const std::string &address)
    : socket(context, ZMQ_PUSH){
	    std::cout << "sending vectors" << std::endl;
		socket.connect(address);      
    }

    // Destructor to clean up the socket
    virtual ~Producer(){
        try{
            socket.close();
        }
        catch (const zmq::error_t &e){
            std::cerr << "Error closing socket: " << e.what() << std::endl;
        }
    }

    // Serialize a vector of type T and send it as a ZeroMQ message
    void produce(const std::vector<T>& vec)
    {
        sendMessage(socket, [vec]() -> zmq::message_t
            {
                uint32_t size = vec.size(); // Store the vector size

                // Allocate a message large enough to hold the size (first 4 bytes) + actual data
                zmq::message_t message(sizeof(uint32_t) + size * sizeof(T));

                // Copy the size of the vector into the first 4 bytes of message
                memcpy(message.data(), &size, sizeof(uint32_t));

                // Copy the actual data after the 4-byte size prefix
                memcpy(static_cast<char*>(message.data()) + sizeof(uint32_t), vec.data(), size * sizeof(T));

                return message;
            });
    }

    private:
    zmq::socket_t socket;
};

// Specialization for std::string
template <>
class Producer<std::string>{
    public:
        Producer(zmq::context_t &context, const std::string &address)
        : socket(context, ZMQ_PUSH){
            socket.bind(address);
        }

        virtual ~Producer(){
            try{
                socket.close();
            }
            catch (const zmq::error_t &e){
                std::cerr << "Error closing socket: " << e.what() << std::endl;
            }
        }

        void produce(const std::string &str){
            sendMessage(socket, [str]() -> zmq::message_t{
                // Serialize the string length and data into a single buffer
                size_t size = str.size();
                zmq::message_t message(sizeof(size_t) + size);

                // Copy the size and data into the message buffer
                memcpy(message.data(), &size, sizeof(size_t));
                memcpy(static_cast<char *>(message.data()) + sizeof(size_t), str.data(), size);

                return message;
            });
        }

    private:
        zmq::socket_t socket;
};

#endif // PRODUCER_HH
