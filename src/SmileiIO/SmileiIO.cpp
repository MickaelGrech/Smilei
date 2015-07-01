/*
 * SmileiIO.cpp
 *
 *  Created on: 3 juil. 2013
 */

#include "SmileiIO.h"

#include <sstream>
#include <iomanip>

#include <mpi.h>

#include "PicParams.h"
#include "Diagnostic.h"
#include "SmileiMPI.h"
#include "SimWindow.h"
#include "ElectroMagn.h"
#include "Species.h"

using namespace std;

SmileiIO::SmileiIO( PicParams& params, Diagnostic& diag, SmileiMPI* smpi ) : 
dump_times(0), 
fieldsToDump(diag.params.fieldsToDump),
time_reference(0.0)
{
    nDim_particle=params.nDim_particle;
    //particleSize = nDim_particle + 3 + 1;
    
    
#ifdef _IO_PARTICLE
    particleSize = nDim_particle + 3 + 1;
    
    ostringstream name("");
    name << "particles-" << setfill('0') << setw(4) << smpi->getRank() << ".h5" ;
	
    hid_t attribute_id;
	
    // Create 1 file containing 1 dataset per Species
    partFile_id = H5Fcreate( name.str().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	
    hsize_t dims[2] = {0, particleSize};
    hsize_t max_dims[2] = {H5S_UNLIMITED, particleSize};
    hid_t file_space = H5Screate_simple(2, dims, max_dims);
	
    hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_layout(plist, H5D_CHUNKED);
    hsize_t chunk_dims[2] = {1, particleSize};
    H5Pset_chunk(plist, 2, chunk_dims);
	
    for (unsigned int ispec=0 ; ispec<params.species_param.size() ; ispec++) {
        ostringstream speciesName("");
        speciesName << params.species_param[ispec].species_type;
        
        //here we check for the presence of multiple ccurence of the same particle name... Souldn't we add a tag for each species?
        unsigned int occurrence=0;
        for (unsigned int iocc=0 ; iocc<ispec ; iocc++) {
            if (params.species_param[ispec].species_type == params.species_param[iocc].species_type)
                occurrence++;
        }
        if (occurrence>0) 
            speciesName << "_" << occurrence;
        
        hid_t did = H5Dcreate(partFile_id, speciesName.str().c_str(), H5T_NATIVE_FLOAT, file_space, H5P_DEFAULT, plist, H5P_DEFAULT);
        partDataset_id.push_back(did);
        
        hid_t tmp_space = H5Screate(H5S_SCALAR);
		
        attribute_id = H5Acreate (partDataset_id[ispec], "Mass", H5T_IEEE_F64BE, tmp_space,H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attribute_id, H5T_NATIVE_DOUBLE, &params.species_param[ispec].mass);
        H5Aclose(attribute_id);
		
        attribute_id = H5Acreate (partDataset_id[ispec], "Charge", H5T_IEEE_F64BE, tmp_space,H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attribute_id, H5T_NATIVE_DOUBLE, &params.species_param[ispec].charge);
        H5Aclose(attribute_id);
		
        H5Sclose(tmp_space);
    }
	
    H5Pclose(plist);
    H5Sclose(file_space);
	
    dims[0] = 1;
    dims[1] = particleSize;
    partMemSpace = H5Screate_simple(2, dims, NULL);
#endif
	
    // ----------------------------
    // Management of global IO file
    // ----------------------------
    MPI_Info info  = MPI_INFO_NULL;
    hid_t plist_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, info);
    
    write_plist = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(write_plist, H5FD_MPIO_INDEPENDENT);
    
    
    // Fields.h5
    // ---------
    global_file_id_  = H5Fcreate( "Fields.h5",     H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
    
    // Create property list for collective dataset write: for Fields.h5
    
    hid_t sid  = H5Screate(H5S_SCALAR);
    hid_t aid = H5Acreate (global_file_id_, "res_time", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, write_plist);
    H5Awrite(aid, H5T_NATIVE_DOUBLE, &(params.res_time));
    H5Sclose(sid);
    H5Aclose(aid);
    
    sid  = H5Screate(H5S_SCALAR);
    aid = H5Acreate (global_file_id_, "every", H5T_NATIVE_UINT, sid, H5P_DEFAULT, write_plist);
    H5Awrite(aid, H5T_NATIVE_UINT, &(diag.params.fieldDump_every));
    H5Sclose(sid);
    H5Aclose(aid);
    
    hsize_t dimsPos = params.res_space.size();
    sid = H5Screate_simple(1, &dimsPos, NULL);
    aid = H5Acreate (global_file_id_, "res_space", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, write_plist);
    H5Awrite(aid, H5T_NATIVE_DOUBLE, &(params.res_space[0]));
    H5Aclose(aid);
    H5Sclose(sid);
    
    dimsPos = params.sim_length.size();
    sid = H5Screate_simple(1, &dimsPos, NULL);
    vector<double> sim_length_norm=params.sim_length;
    transform(sim_length_norm.begin(), sim_length_norm.end(), sim_length_norm.begin(),bind1st(multiplies<double>(),1.0/params.conv_fac));
    
    aid = H5Acreate (global_file_id_, "sim_length", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, write_plist);
    H5Awrite(aid, H5T_NATIVE_DOUBLE, &(sim_length_norm[0]));
    H5Aclose(aid);
    H5Sclose(sid);
    
    
    // Fields_avg.h5
    // -------------
    global_file_id_avg = 0;
    if  (diag.params.ntime_step_avg!=0) {
        global_file_id_avg = H5Fcreate( "Fields_avg.h5", H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
        
        // Create property list for collective dataset write: for Fields.h5
        hid_t sid  = H5Screate(H5S_SCALAR);
        hid_t aid = H5Acreate (global_file_id_avg, "res_time", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, write_plist);
        H5Awrite(aid, H5T_NATIVE_DOUBLE, &(params.res_time));
        H5Sclose(sid);
        H5Aclose(aid);
        
        sid  = H5Screate(H5S_SCALAR);
        aid = H5Acreate (global_file_id_avg, "every", H5T_NATIVE_UINT, sid, H5P_DEFAULT, write_plist);
        H5Awrite(aid, H5T_NATIVE_UINT, &(diag.params.fieldDump_every));
        H5Sclose(sid);
        H5Aclose(aid);
        
        hsize_t dimsPos = params.res_space.size();
        sid = H5Screate_simple(1, &dimsPos, NULL);
        aid = H5Acreate (global_file_id_avg, "res_space", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, write_plist);
        H5Awrite(aid, H5T_NATIVE_DOUBLE, &(params.res_space[0]));
        H5Aclose(aid);
        H5Sclose(sid);
        
        dimsPos = params.sim_length.size();
        sid = H5Screate_simple(1, &dimsPos, NULL);
        vector<double> sim_length_norm=params.sim_length;
        transform(sim_length_norm.begin(), sim_length_norm.end(), sim_length_norm.begin(),bind1st(multiplies<double>(),1.0/params.conv_fac));
        
        aid = H5Acreate (global_file_id_avg, "sim_length", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, write_plist);
        H5Awrite(aid, H5T_NATIVE_DOUBLE, &(sim_length_norm[0]));
        H5Aclose(aid);
        H5Sclose(sid);
    }
    
    H5Pclose(plist_id);
    
    time_reference=time_seconds();
    
}

SmileiIO::~SmileiIO()
{
    // Management of global IO file
    H5Fclose( global_file_id_ );
    
    // Management of global IO file
    if (global_file_id_avg != 0)
        H5Fclose( global_file_id_avg );
	
#ifdef _IO_PARTICLE
    H5Sclose(partMemSpace);
    for ( unsigned int s=0 ; s<partDataset_id.size() ; s++ )
        H5Dclose(partDataset_id[s]);
    H5Fclose(partFile_id);
#endif	
    H5Pclose( write_plist );
}

// ---------------------------------------------------------------------------------------------------------------------
// Write all fields of all time step in the same file
// ---------------------------------------------------------------------------------------------------------------------
void SmileiIO::writeAllFieldsSingleFileTime( ElectroMagn* EMfields, int time )
{
    ostringstream name_t;
    name_t.str("");
    name_t << "/" << setfill('0') << setw(10) << time;
	
    DEBUG(10,"[hdf] GROUP _________________________________ " << name_t.str());
    hid_t group_id = H5Gcreate(global_file_id_, name_t.str().c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	
    for (vector<Field*>::iterator iterField=EMfields->allFields.begin(); iterField!=EMfields->allFields.end(); iterField++) {
        if (fieldsToDump.empty())
            writeFieldsSingleFileTime( *iterField, group_id );
        else
            for (vector<string>::iterator iterName=fieldsToDump.begin(); iterName!=fieldsToDump.end(); iterName++) 
                if ((*iterField)->name==(*iterName)) {
                    writeFieldsSingleFileTime( *iterField, group_id );
                }
    }
	
    H5Gclose(group_id);
	
    H5Fflush( global_file_id_, H5F_SCOPE_GLOBAL );
	
}



// ---------------------------------------------------------------------------------------------------------------------
// Write all fields of all time step in the same file
// ---------------------------------------------------------------------------------------------------------------------
void SmileiIO::writeAvgFieldsSingleFileTime( ElectroMagn* EMfields, int time )
{
    ostringstream name_t;
    name_t.str("");
    name_t << "/" << setfill('0') << setw(10) << time;
	
    DEBUG(10,"[hdf] GROUP _________________________________ " << name_t.str());
    hid_t group_id = H5Gcreate(global_file_id_avg, name_t.str().c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    
    for (vector<Field*>::iterator iterField=EMfields->allFields_avg.begin(); iterField!=EMfields->allFields_avg.end(); iterField++) {
        if (fieldsToDump.empty())
            writeFieldsSingleFileTime( *iterField, group_id );
        else
            for (vector<string>::iterator iterName=fieldsToDump.begin(); iterName!=fieldsToDump.end(); iterName++) 
                if ((*iterField)->name==(*iterName)) {
                    writeFieldsSingleFileTime( *iterField, group_id );
                }
    }
	
    H5Gclose(group_id);
	
    H5Fflush( global_file_id_avg, H5F_SCOPE_GLOBAL );
	
}




// ---------------------------------------------------------------------------------------------------------------------
// Each MPI process writes is particles in its own file, data are overwritten at each call ( particles-MPI_Rank.h5 )
// In progress ...
// ---------------------------------------------------------------------------------------------------------------------
void SmileiIO::writePlasma( vector<Species*> vecSpecies, double time, SmileiMPI* smpi )
{
	
#ifdef _IO_PARTICLE
    if (smpi->isMaster()) DEBUG("write species disabled");
    return;
	
    for (int ispec=0 ; ispec<vecSpecies.size(); ispec++) {
        Particles* cuParticles = &(vecSpecies[ispec])->particles;
        MESSAGE(2,"write species " << ispec);
		
        for (unsigned int p=0; p<(vecSpecies[ispec])->getNbrOfParticles(); p++ ) {
			
            hid_t file_space = H5Dget_space(partDataset_id[ispec]);
            hsize_t dimsO[2];
            H5Sget_simple_extent_dims(file_space, dimsO, NULL);
            H5Sclose(file_space);
            hsize_t dims[2];
            dims[0] = dimsO[0]+1;
            dims[1] = dimsO[1];
            H5Dset_extent(partDataset_id[ispec], dims);
			
            file_space = H5Dget_space(partDataset_id[ispec]);
            hsize_t start[2];
            hsize_t count[2] = {1, particleSize};
            start[0] = dimsO[0];
            start[1] = 0;
            H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
            H5Dwrite(partDataset_id[ispec], H5T_NATIVE_DOUBLE, partMemSpace, file_space, H5P_DEFAULT, &((*cuParticles)[ p ]->position(0)));
            H5Sclose(file_space);
			
			
        } // End for p
		
    } // End for ispec
	
#endif
}

bool SmileiIO::dump( ElectroMagn* EMfields, unsigned int itime, bool signal_received, std::vector<Species*> vecSpecies, SmileiMPI* smpi, SimWindow* simWindow, PicParams &params, InputData& input_data) { 
    if (signal_received ||
        (params.dump_step != 0 && (itime % params.dump_step == 0)) ||
        (params.dump_minutes != 0.0 && time_seconds()/60.0 > smpi->getSize()*(params.dump_minutes*(dump_times+1))) ) {
        dumpAll( EMfields, itime,  vecSpecies, smpi, simWindow, params, input_data);
        if (signal_received || params.exit_after_dump)	return true;
    }
    return false;
}

void SmileiIO::dumpAll( ElectroMagn* EMfields, unsigned int itime,  std::vector<Species*> vecSpecies, SmileiMPI* smpi, SimWindow* simWin, PicParams &params, InputData& input_data) { 
	hid_t fid, gid, sid, aid, did, tid;
    
	ostringstream nameDump("");
	nameDump << "dump-" << setfill('0') << setw(4) << dump_times%params.dump_file_sequence << "-" << setfill('0') << setw(4) << smpi->getRank() << ".h5" ;
	fid = H5Fcreate( nameDump.str().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	dump_times++;
	
	MESSAGE(2, "DUMPING fields and particles " << nameDump.str());
    
	
	sid  = H5Screate(H5S_SCALAR);
    tid = H5Tcopy(H5T_C_S1);
    H5Tset_size(tid, input_data.namelist.size());
    H5Tset_strpad(tid,H5T_STR_NULLTERM);
    aid = H5Acreate(fid, "Namelist", tid, sid, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(aid, tid, input_data.namelist.c_str());	
    H5Aclose(aid);
    H5Sclose(sid);
    H5Tclose(tid);
	
	sid = H5Screate(H5S_SCALAR);	
	aid = H5Acreate(fid, "dump_step", H5T_NATIVE_UINT, sid, H5P_DEFAULT, H5P_DEFAULT);
	H5Awrite(aid, H5T_NATIVE_UINT, &itime);
	H5Sclose(sid);
	H5Aclose(aid);
	
	
	dumpFieldsPerProc(fid, EMfields->Ex_);
    dumpFieldsPerProc(fid, EMfields->Ey_);
    dumpFieldsPerProc(fid, EMfields->Ez_);
    dumpFieldsPerProc(fid, EMfields->Bx_);
    dumpFieldsPerProc(fid, EMfields->By_);
    dumpFieldsPerProc(fid, EMfields->Bz_);
    if (EMfields->Ex_avg!=NULL) {
        dumpFieldsPerProc(fid, EMfields->Ex_avg);
        dumpFieldsPerProc(fid, EMfields->Ey_avg);
        dumpFieldsPerProc(fid, EMfields->Ez_avg);
        dumpFieldsPerProc(fid, EMfields->Bx_avg);
        dumpFieldsPerProc(fid, EMfields->By_avg);
        dumpFieldsPerProc(fid, EMfields->Bz_avg);
    }
	
    H5Fflush( fid, H5F_SCOPE_GLOBAL );
	
	sid = H5Screate(H5S_SCALAR);
	aid = H5Acreate(fid, "species", H5T_NATIVE_UINT, sid, H5P_DEFAULT, H5P_DEFAULT);
	unsigned int vecSpeciesSize=vecSpecies.size();
	H5Awrite(aid, H5T_NATIVE_UINT, &vecSpeciesSize);
	H5Aclose(aid);
	H5Sclose(sid);
	
	
    for (unsigned int ispec=0 ; ispec<vecSpecies.size() ; ispec++) {
		ostringstream name("");
		name << setfill('0') << setw(2) << ispec;
		string groupName="species-"+name.str()+"-"+vecSpecies[ispec]->species_param.species_type;
		gid = H5Gcreate(fid, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        
		sid = H5Screate(H5S_SCALAR);
		aid = H5Acreate(gid, "partCapacity", H5T_NATIVE_UINT, sid, H5P_DEFAULT, H5P_DEFAULT);
		unsigned int partCapacity=vecSpecies[ispec]->particles.capacity();
		H5Awrite(aid, H5T_NATIVE_UINT, &partCapacity);
		H5Aclose(aid);
		H5Sclose(sid);
        
		sid = H5Screate(H5S_SCALAR);
		aid = H5Acreate(gid, "partSize", H5T_NATIVE_UINT, sid, H5P_DEFAULT, H5P_DEFAULT);
		unsigned int partSize=vecSpecies[ispec]->particles.size();
		H5Awrite(aid, H5T_NATIVE_UINT, &partSize);
		H5Aclose(aid);
		H5Sclose(sid);
		
		if (partSize>0) {
			hsize_t dimsPart[1] = {vecSpecies[ispec]->getNbrOfParticles()};
			
			for (unsigned int i=0; i<vecSpecies[ispec]->particles.Position.size(); i++) {
				ostringstream namePos("");
				namePos << "Position-" << i;
				sid = H5Screate_simple(1, dimsPart, NULL);
				did = H5Dcreate(gid, namePos.str().c_str(), H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
				H5Dwrite(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->particles.Position[i][0]);
				H5Dclose(did);
				H5Sclose(sid);
			}
			
			for (unsigned int i=0; i<vecSpecies[ispec]->particles.Momentum.size(); i++) {
				ostringstream namePos("");
				namePos << "Momentum-" << i;
				sid = H5Screate_simple(1, dimsPart, NULL);
				did = H5Dcreate(gid, namePos.str().c_str(), H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
				H5Dwrite(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->particles.Momentum[i][0]);
				H5Dclose(did);
				H5Sclose(sid);
			}
			
			sid = H5Screate_simple(1, dimsPart, NULL);
			did = H5Dcreate(gid, "Weight", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
			H5Dwrite(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->particles.Weight[0]);
			H5Dclose(did);
			H5Sclose(sid);
			
			sid = H5Screate_simple(1, dimsPart, NULL);
			did = H5Dcreate(gid, "Charge", H5T_NATIVE_SHORT, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
			H5Dwrite(did, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->particles.Charge[0]);
			H5Dclose(did);
			H5Sclose(sid);
            
            
			hsize_t dimsbmin[1] = {vecSpecies[ispec]->bmin.size()};
			sid = H5Screate_simple(1, dimsbmin, NULL);
			did = H5Dcreate(gid, "bmin", H5T_NATIVE_UINT, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
			H5Dwrite(did, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->bmin[0]);
			H5Dclose(did);
			H5Sclose(sid);
            
			hsize_t dimsbmax[1] = {vecSpecies[ispec]->bmax.size()};
			sid = H5Screate_simple(1, dimsbmax, NULL);
			did = H5Dcreate(gid, "bmax", H5T_NATIVE_UINT, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
			H5Dwrite(did, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->bmax[0]);
			H5Dclose(did);
			H5Sclose(sid);
            
		}
		H5Gclose(gid);
    }
    
    // Dump moving window status
    if (simWin!=NULL)
        dumpMovingWindow(fid, simWin);
    
    H5Fclose( fid );
	
};

void SmileiIO::dumpFieldsPerProc(hid_t fid, Field* field)
{
	hsize_t dims[1]={field->globalDims_};
	hid_t sid = H5Screate_simple (1, dims, NULL);	
	hid_t did = H5Dcreate (fid, field->name.c_str(), H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
	H5Dwrite(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &field->data_[0]);
	H5Dclose (did);	
	H5Sclose(sid);
}

void SmileiIO::dumpMovingWindow(hid_t fid, SimWindow* simWin)
{  
    double x_moved = simWin->getXmoved();
    
    hid_t sid = H5Screate(H5S_SCALAR);	
    hid_t aid = H5Acreate(fid, "x_moved", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(aid, H5T_NATIVE_DOUBLE, &x_moved);
    H5Sclose(sid);
    H5Aclose(aid);
    
}


void SmileiIO::restartAll( ElectroMagn* EMfields, unsigned int &itime,  std::vector<Species*> &vecSpecies, SmileiMPI* smpi, SimWindow* simWin, PicParams &params, InputData& input_data) { 
	
	string nameDump("");
	
	// This will open both dumps and pick the last one
	for (unsigned int i=0;i<params.dump_file_sequence; i++) {
		ostringstream nameDumpTmp("");
		nameDumpTmp << "dump-" << setfill('0') << setw(4) << i << "-" << setfill('0') << setw(4) << smpi->getRank() << ".h5" ;
		ifstream f(nameDumpTmp.str().c_str());
		if (f.good()) {
			hid_t fid = H5Fopen( nameDumpTmp.str().c_str(), H5F_ACC_RDWR, H5P_DEFAULT);			
			hid_t aid = H5Aopen(fid, "dump_step", H5T_NATIVE_UINT);
			unsigned int itimeTmp=0;
			H5Aread(aid, H5T_NATIVE_UINT, &itimeTmp);	
			H5Aclose(aid);
			H5Fclose(fid);
			if (itimeTmp>itime) {
				itime=itimeTmp;
				nameDump=nameDumpTmp.str();
				dump_times=i;
			}
		}
		f.close();
	}
	
	if (nameDump.empty()) ERROR("Cannot find a valid restart file");
	
	MESSAGE(2, "RESTARTING fields and particles " << nameDump);
    
	hid_t fid = H5Fopen( nameDump.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
	
	hid_t aid, gid, did, sid;
	
	aid = H5Aopen(fid, "dump_step", H5T_NATIVE_UINT);
	H5Aread(aid, H5T_NATIVE_UINT, &itime);	
	H5Aclose(aid);
	
	restartFieldsPerProc(fid, EMfields->Ex_);
    restartFieldsPerProc(fid, EMfields->Ey_);
    restartFieldsPerProc(fid, EMfields->Ez_);
    restartFieldsPerProc(fid, EMfields->Bx_);
    restartFieldsPerProc(fid, EMfields->By_);
    restartFieldsPerProc(fid, EMfields->Bz_);
    if (EMfields->Ex_avg!=NULL) {
        restartFieldsPerProc(fid, EMfields->Ex_avg);
        restartFieldsPerProc(fid, EMfields->Ey_avg);
        restartFieldsPerProc(fid, EMfields->Ez_avg);
        restartFieldsPerProc(fid, EMfields->Bx_avg);
        restartFieldsPerProc(fid, EMfields->By_avg);
        restartFieldsPerProc(fid, EMfields->Bz_avg);
    }
	
	aid = H5Aopen(fid, "species", H5T_NATIVE_UINT);
	unsigned int vecSpeciesSize=0;
	H5Aread(aid, H5T_NATIVE_UINT, &vecSpeciesSize);
	H5Aclose(aid);	
	if (vecSpeciesSize != vecSpecies.size()) {
		ERROR("Number of species differs between dump (" << vecSpeciesSize << ") and namelist ("<<vecSpecies.size()<<")");
	}
	
	
	for (unsigned int ispec=0 ; ispec<vecSpecies.size() ; ispec++) {
		ostringstream name("");
		name << setfill('0') << setw(2) << ispec;
		string groupName="species-"+name.str()+"-"+vecSpecies[ispec]->species_param.species_type;
		gid = H5Gopen(fid, groupName.c_str(),H5P_DEFAULT);
		
		aid = H5Aopen(gid, "partCapacity", H5T_NATIVE_UINT);
		unsigned int partCapacity=0;
		H5Aread(aid, H5T_NATIVE_UINT, &partCapacity);
		H5Aclose(aid);
		vecSpecies[ispec]->particles.reserve(partCapacity,nDim_particle);		
        
		aid = H5Aopen(gid, "partSize", H5T_NATIVE_UINT);
		unsigned int partSize=0;
		H5Aread(aid, H5T_NATIVE_UINT, &partSize);
		H5Aclose(aid);	
		vecSpecies[ispec]->particles.initialize(partSize,nDim_particle);		
		
		
		if (partSize>0) {
			for (unsigned int i=0; i<vecSpecies[ispec]->particles.Position.size(); i++) {
				ostringstream namePos("");
				namePos << "Position-" << i;
				did = H5Dopen(gid, namePos.str().c_str(), H5P_DEFAULT);
				H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->particles.Position[i][0]);
				H5Dclose(did);
			}
			
			for (unsigned int i=0; i<vecSpecies[ispec]->particles.Momentum.size(); i++) {
				ostringstream namePos("");
				namePos << "Momentum-" << i;
				did = H5Dopen(gid, namePos.str().c_str(), H5P_DEFAULT);
				H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->particles.Momentum[i][0]);
				H5Dclose(did);
			}
			
			did = H5Dopen(gid, "Weight", H5P_DEFAULT);
			H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->particles.Weight[0]);
			H5Dclose(did);
			
			did = H5Dopen(gid, "Charge", H5P_DEFAULT);
			H5Dread(did, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->particles.Charge[0]);
			H5Dclose(did);
            
			did = H5Dopen(gid, "bmin", H5P_DEFAULT);
			sid = H5Dget_space(did);
			
			int ndims=H5Sget_simple_extent_ndims(sid);
			vector<hsize_t> dims(ndims);
			H5Sget_simple_extent_dims(sid,&dims[0],NULL);
			
			vecSpecies[ispec]->bmin.resize(dims[0]);
			H5Dread(did, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->bmin[0]);
			H5Dclose(did);
			H5Sclose(sid);
			
			did = H5Dopen(gid, "bmax", H5P_DEFAULT);
			sid = H5Dget_space(did);
			H5Sget_simple_extent_dims(sid,&dims[0],NULL);
			
			vecSpecies[ispec]->bmin.resize(dims[0]);
			H5Dread(did, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vecSpecies[ispec]->bmax[0]);
			H5Dclose(did);
			H5Sclose(sid);
		}
		
		H5Gclose(gid);
    }
	
    // load window status
    if (simWin!=NULL)
        restartMovingWindow(fid, simWin);
    
	H5Fclose( fid );
};

void SmileiIO::restartFieldsPerProc(hid_t fid, Field* field)
{
	hsize_t dims[1]={field->globalDims_};
	hid_t sid = H5Screate_simple (1, dims, NULL);
	hid_t did = H5Dopen (fid, field->name.c_str(),H5P_DEFAULT);
	H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &field->data_[0]);
	H5Dclose (did);
	H5Sclose(sid);
}

void SmileiIO::restartMovingWindow(hid_t fid, SimWindow* simWin)
{  
    hid_t aid = H5Aopen(fid, "x_moved", H5T_NATIVE_DOUBLE);
    double x_moved=0.;
    H5Aread(aid, H5T_NATIVE_DOUBLE, &x_moved);	
    H5Aclose(aid);
    
    simWin->setXmoved(x_moved);
    
}

double SmileiIO::time_seconds() {
	double time_temp = MPI_Wtime();	
	double time_sec=0;
	MPI_Allreduce(&time_temp,&time_sec,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	return (time_sec-time_reference);
}
