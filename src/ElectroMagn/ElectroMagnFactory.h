#ifndef ELECTROMAGNFACTORY_H
#define ELECTROMAGNFACTORY_H

#include <sstream>
#include "ElectroMagn.h"
#include "ElectroMagn1D.h"
#include "ElectroMagn2D.h"

#include "Patch.h"
#include "Params.h"

#include "Tools.h"

class ElectroMagnFactory {
public:
    static ElectroMagn* create(Params& params, std::vector<Species*>& vecSpecies,  Patch* patch) {
        ElectroMagn* EMfields = NULL;
        if ( params.geometry == "1d3v" ) {
	  EMfields = new ElectroMagn1D(params, vecSpecies, patch);
        }
        else if ( params.geometry == "2d3v" ) {
	  EMfields = new ElectroMagn2D(params, vecSpecies, patch);
        }
        else {
            ERROR( "Unknwon geometry : " << params.geometry );
        }

        
        // Fill allfields
        EMfields->allFields.push_back(EMfields->Ex_ );
        EMfields->allFields.push_back(EMfields->Ey_ );
        EMfields->allFields.push_back(EMfields->Ez_ );
        EMfields->allFields.push_back(EMfields->Bx_ );
        EMfields->allFields.push_back(EMfields->By_ );
        EMfields->allFields.push_back(EMfields->Bz_ );
        EMfields->allFields.push_back(EMfields->Bx_m);
        EMfields->allFields.push_back(EMfields->By_m);
        EMfields->allFields.push_back(EMfields->Bz_m);
        EMfields->allFields.push_back(EMfields->Jx_ );
        EMfields->allFields.push_back(EMfields->Jy_ );
        EMfields->allFields.push_back(EMfields->Jz_ );
        EMfields->allFields.push_back(EMfields->rho_);

        for (unsigned int ispec=0; ispec<vecSpecies.size(); ispec++) {
            EMfields->allFields.push_back(EMfields->Jx_s[ispec] );
            EMfields->allFields.push_back(EMfields->Jy_s[ispec] );
            EMfields->allFields.push_back(EMfields->Jz_s[ispec] );
            EMfields->allFields.push_back(EMfields->rho_s[ispec]);
        }
                    
        EMfields->allFields_avg.push_back(EMfields->Ex_avg);
        EMfields->allFields_avg.push_back(EMfields->Ey_avg);
        EMfields->allFields_avg.push_back(EMfields->Ez_avg);
        EMfields->allFields_avg.push_back(EMfields->Bx_avg);
        EMfields->allFields_avg.push_back(EMfields->By_avg);
        EMfields->allFields_avg.push_back(EMfields->Bz_avg);
        
        std::stringstream ss;
        for (std::vector<Field*>::iterator iterField=EMfields->allFields.begin(); iterField!=EMfields->allFields.end(); iterField++) {
            ss << (*iterField)->name << " ";
        }
        if (patch->isMaster()) {
	    MESSAGE(1,"EM fields dump      :");
	    MESSAGE(2, ss.str() );
        }
        ss.str("");
        for (std::vector<Field*>::iterator iterField=EMfields->allFields_avg.begin(); iterField!=EMfields->allFields_avg.end(); iterField++) {
            ss << (*iterField)->name << " ";
        }
        if (patch->isMaster()) {
            MESSAGE(1,"EM avg. fields dump :");
	    MESSAGE(2, ss.str() );
        }
        
        return EMfields;
    }

};

#endif

