#ifndef _scan_h_
#define _scan_h_
#include <rose.h>
#include <rose_p28.h>
#include <stp_schema.h>

class MyURIManager : public RoseManager {
protected:
	RoseReference * 	f_ref;

public:
	MyURIManager() : f_ref(0) {}

	ROSE_DECLARE_MANAGER_COMMON();

	RoseReference * should_go_to_uri()			{ return f_ref; }
	void should_go_to_uri(RoseReference * r)		{ f_ref = r; }

	static MyURIManager * find(RoseObject *);
	static MyURIManager * make(RoseObject *);
};

void update_uri_forwarding(RoseDesign * design);


class MyPDManager : public RoseManager{
public:
	RoseDesign* childDes;
	stp_product_definition* childPD;

	void hasChild(stp_product_definition * c)	{ childPD = c; }
	void hasChildIn(RoseDesign * d)	{ childDes = d; }

	MyPDManager() { childPD = NULL; childDes = NULL; }

	static MyPDManager * find(RoseObject * nauo);
	static MyPDManager * make(RoseObject *);

	ROSE_DECLARE_MANAGER_COMMON();
};


#endif