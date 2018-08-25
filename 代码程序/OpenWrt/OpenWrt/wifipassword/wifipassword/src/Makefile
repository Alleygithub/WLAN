wifipassword : base64.o sm3.o wificonfiguration.o passwordmaintaince.o main.o
	$(CXX) -o wifipassword base64.o sm3.o wificonfiguration.o passwordmaintaince.o main.o

sm3.o : sm3.h typedef.h
	$(CXX) -c sm3.cpp -o sm3.o
base64.o : base64.h typedef.h
	$(CXX) -c base64.cpp -o base64.o
wificonfiguration.o : wificonfiguration.h typedef.h
	$(CXX) -c wificonfiguration.cpp -o wificonfiguration.o
passwordmaintaince.o : passwordmaintaince.h wificonfiguration.h base64.h sm3.h typedef.h
	$(CXX) -c passwordmaintaince.cpp -o passwordmaintaince.o
main.o : passwordmaintaince.h
	$(CXX) -c main.cpp -o main.o

clean:
	rm -f sm3.o base64.o wificonfiguration.o passwordmaintaince.o main.o wifipassword
