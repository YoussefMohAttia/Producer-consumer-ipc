all: producer consumer

producer: producer.cpp
	g++ producer.cpp -o producer
	
consumer: consumer.cpp
	g++ consumer.cpp -o consumer

clean:
	@rm -f producer consumer
