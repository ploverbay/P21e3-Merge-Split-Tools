//Chris Made this code.
//6/2/14

#include <rose.h>
#include <rose_p28.h>
#include <stp_schema.h>
#include <stix.h>
#include <string>
#include <map>
#include <iostream>
#include <ARM.h>
#include "scan.h"
#include <ctype.h>

#pragma comment(lib,"stpcad_stix.lib")

static void copy_header(RoseDesign * dst, RoseDesign * src)
{
	unsigned i, sz;
	// Copy over the header information from the original
	dst->initialize_header();
	dst->header_name()->originating_system(src->header_name()->originating_system());
	dst->header_name()->authorisation(src->header_name()->authorisation());
	for (i = 0, sz = src->header_name()->author()->size(); i<sz; i++)
		dst->header_name()->author()->add(
		src->header_name()->author()->get(i)
		);

	for (i = 0, sz = src->header_name()->author()->size(); i<sz; i++)
		dst->header_name()->organization()->add(
		src->header_name()->organization()->get(i)
		);

	RoseStringObject desc = "Extracted from STEP assembly: ";
	desc += src->name();
	desc += ".";
	desc += src->fileExtension();
	dst->header_description()->description()->add(desc);
}

static void copy_schema(RoseDesign * dst, RoseDesign * src)
{
	// Make the new files the same schema unless the original AP does
	// not have the external reference definitions.
	//
	switch (stplib_get_schema(src)) {
	case stplib_schema_ap203e2:
	case stplib_schema_ap214:
	case stplib_schema_ap242:
		stplib_put_schema(dst, stplib_get_schema(src));
		break;

	case stplib_schema_ap203:
	default:
		stplib_put_schema(dst, stplib_schema_ap214);
		break;
	}
}

void addRefAndAnchor(RoseObject * obj, RoseDesign * ProdOut, RoseDesign * master, std::string dir){ //obj from output file, and master fiel for putting refs into
	std::string ProdOutName;
	ProdOutName.append(obj->domain()->name());
	ProdOutName.append("_split_item");
	ProdOutName.append(std::to_string(obj->entity_id()));
	//ProdOutName = SafeName(ProdOutName);

	ProdOut->addName(ProdOutName.c_str(), obj);

	std::string refURI = (std::string("geo.stp#") + ProdOutName);//uri for created reference to prod/obj

	RoseReference *ref = rose_make_ref(master, refURI.c_str());
	ref->resolved(obj);
	MyURIManager *URIManager;	//Make an instance of the class which handles updating URIS
	URIManager = MyURIManager::make(obj);
	URIManager->should_go_to_uri(ref);
}

void deleteRefandAnchor(RoseReference * ref, RoseDesign * geo){
	//delete anchor from geometry
	std::string URI(ref->uri());
	int poundpos = URI.find_first_of('#');
	std::string anchor = URI.substr(poundpos + 1);	//anchor contains "item1234"
	geo->removeName(anchor.c_str());

	//delete reference 
	rose_move_to_trash(ref);
}

//findobjectinworkspace
void removeExtraRefandAnchor(RoseObject * obj, RoseDesign * geo){
	unsigned i, sz, sz2;
	RoseReference * ref = ROSE_CAST(RoseReference, obj);
	RoseRefUsage *rru = ref->usage();	//rru is a linked list of all the objects that use ref

	//find object being referenced
	std::string URI(ref->uri());
	int poundpos = URI.find_first_of('#');
	std::string anchor = URI.substr(poundpos + 1);
	RoseObject *rObj = geo->findObject(anchor.c_str());

	if (!rose_is_marked(rObj)){ //delete references with no useage, or a useage that only contains itself
		if (!rru){
			deleteRefandAnchor(ref, geo);
		}
		else{
			//count reference usages
			int counter = 0;
			if (rru->user()->entity_id()) { //if first usage isn't the reference
				counter++; 
			}
			while (rru = rru->next_for_ref()){
				if (rru->user()){
					counter++;
				}
			}


			//if not used by anything other than itself, delete reference
			if (counter == 0){ deleteRefandAnchor(ref, geo); }
		}
	}
	else{
		ListOfRoseObject parents;
		bool deleteRef = true;

		//if reference has at least 1 parent in PMI(not geo) keep it otherwise, remove it		
		RoseDesign * PMI = ref->design();
		ref->usedin(NULL, NULL, &parents);
		unsigned count = 0;
		for (i = 0, sz2 = parents.size(); i < sz2; i++){
			RoseObject * parent = parents.get(i);
			if (parent->entity_id() == 0) { count++; }
			if (parent->design() != geo){
				deleteRef = false;
				
			}
		}
		int counter = 0;
		if (rru){
			if (rru->user()->entity_id()) {
				counter++;
			}
			while (rru = rru->next_for_ref()){
				if (rru->user()){
					counter++;
				}
			}	
		}
		if (counter){ deleteRef = false; }
		//std::cout << counter << ", " << sz2 << ", " << rObj->domain()->name() << std::endl;
		if ( (sz2 - count) < 1) { deleteRef = false; } //std::cout << rObj->domain()->name() << " has " << sz << " parents and is NOT being removed" << std::endl; }
		if (deleteRef) { deleteRefandAnchor(ref, geo); }//std::cout << " and has " << sz << "," << sz2 << ":" << count << " parents and is being removed" << std::endl; }
	} 
}

int main(int argc, char* argv[])
{
	
	stplib_init();	// initialize merged cad library
	//rose_p28_init();	// support xml read/write
	
	FILE *out;
	out = fopen("log.txt", "w");
	ROSE.error_reporter()->error_file(out);
	RoseP21Writer::max_spec_version(PART21_ED3);	//We need to use Part21 Edition 3 otherwise references won't be handled properly.
	ST_MODULE_FORCE_LOAD();

//##################### INITILIZE DESIGNS #####################
	printf("Reading file '%s'\n", "sp3-boxy_fmt_original.stp");
	// Read a STEP file and dimension the workpiece in that file
	RoseDesign *des = ROSE.useDesign("sp3-boxy_fmt_original.stp");
	des->saveAs("PMI.stp");
	RoseDesign *PMI = ROSE.useDesign("PMI.stp");

	RoseDesign *geo = pnew RoseDesign;
	geo->name(std::string (des->name() + std::string("_Geometry")).c_str());
	geo->saveAs("geo.stp");
	geo = ROSE.useDesign("geo.stp");
	//copy_schema(geo, PMI);
	//copy_header(geo, PMI);
	stix_tag_units(PMI);
	ARMpopulate(PMI);
//#############################################################

	//STModule
	ARMCursor cur; //arm cursor
	ARMObject *a_obj;
	cur.traverse(PMI);
	
	Workpiece_IF  *workpiece = NULL;

	ListOfRoseObject *aimObjs = pnew ListOfRoseObject;
	rose_mark_begin();
	rose_compute_backptrs(PMI);
//Creates all references that may be necessary
	while ((a_obj = cur.next())) {
		
		workpiece = a_obj->castToWorkpiece_IF();
		if (workpiece) {
			unsigned i, sz;
			RoseObject * aimObj;
			
			a_obj->getAIMObjects(aimObjs);

			ARMresolveReferences(aimObjs);
			
			for (i = 0, sz = aimObjs->size(); i < sz; i++){
				aimObj = aimObjs->get(i);
			
				//moves evyerthing
				aimObj->move(geo, 0);
				addRefAndAnchor(aimObj, geo, PMI, ""); //old ref creations, made too many refs and had repeats
				
				ListOfRoseObject parents;
				aimObj->usedin(NULL, NULL, &parents);
				if (parents.size() < 1){ rose_mark_set(aimObj); }//mark aim obj with no parents
				
			}		
		}
	}
	
	update_uri_forwarding(PMI);

//Removes uneccesary References
	RoseCursor curser;
	curser.traverse(PMI->reference_section());
	curser.domain(ROSE_DOMAIN(RoseReference));
	RoseObject * obj;
	int count = 0;
	//std::cout << "Curser size: " << curser.size() << std::endl;
	while (obj = curser.next()){
		removeExtraRefandAnchor(obj, geo);
	}
	rose_mark_end();
	
	
	ARMgc(PMI);
	ARMgc(geo);
	rose_empty_trash();
	geo->save();
	PMI->save();
	

	ARMsave(geo);
	ARMsave(PMI);
	rose_release_backptrs(PMI);
	return 0;
}