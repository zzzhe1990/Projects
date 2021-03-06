#include "PSFOutput.h"
#include "Molecules.h"
#include <cstdio>

using namespace mol_setup;

namespace{
    const char* remarkHeader = "!NTITLE";
    const char* remarkTag = " REMARKS ";
    const char* atomHeader = "!NATOM";
    const char* bondHeader = "!NBOND: bonds";
    const char* angleHeader = "!NTHETA: angles";
    const char* dihedralHeader = "!NPHI: dihedrals";

    const char* headerFormat = "%8d %s \n";
    //atom ID, segment name, residue ID, residue name, 
    //atom name, atom type, charge, mass, and an unused 0
    const char* atomFormat = "%8d%4s%3d%7s%4s%6s%12.6f%14.4f%12d\n";

    const int bondPerLine = 4;
    const int anglePerLine = 3;
    const int dihPerLine = 2;
}

PSFOutput::PSFOutput(const Molecules& molecules, mol_setup::MolMap& molMap, 
                     const std::vector<std::string>& kindNames) : molecules(&molecules), molNames(kindNames)
{
    molKinds.resize(molMap.size());
    for(uint i = 0; i < kindNames.size(); ++i)
    {
        molKinds[i] = molMap[kindNames[i]];
    }
    CountMolecules();
}

void PSFOutput::CountMolecules()
{
    //count molecules of each kind
    std::vector<uint> molCounts(molKinds.size(), 0);
    for(uint i = 0; i < molecules->count; ++i)
    {
        ++molCounts[molecules->kIndex[i]];
    }
    totalAngles = 0;
    totalAtoms = 0;
    totalBonds = 0;
    totalDihs = 0;
    for(uint k = 0; k < molCounts.size(); ++k)
    {
        totalAtoms += molCounts[k] * molKinds[k].atoms.size();
        totalBonds += molCounts[k] * molKinds[k].bonds.size() / 2;
        totalAngles += molCounts[k] * molKinds[k].angles.size() / 3;
        totalDihs += molCounts[k] * molKinds[k].dihedrals.size() / 4;
    }
}

void PSFOutput::PrintPSF(const std::string& filename) const
{
    std::vector<std::string> remarks;
    //default file remarks
    remarks.push_back("Combined PSF produced by GOMC");
    remarks.push_back("Contains Geometry data for molecules in ALL boxes in the system");
    PrintPSF(filename, remarks);
}

void PSFOutput::PrintPSF(const std::string& filename, const std::vector<std::string>& remarks) const
{
    FILE* outfile = fopen(filename.c_str(), "w");
    if (outfile == NULL)
    {
        fprintf(stderr, "Error opening PSF output file %s", filename.c_str());
        return;
    }

    fprintf(outfile, "PSF\n\n");
    PrintRemarks(outfile, remarks);
    PrintAtoms(outfile);
    PrintBonds(outfile);
    PrintAngles(outfile);
    PrintDihedrals(outfile);
    fclose(outfile);
}

void PSFOutput::PrintRemarks(FILE* outfile, const std::vector<std::string>& remarks) const
{
    fprintf(outfile, headerFormat, remarks.size(), remarkHeader);
    for(uint i = 0; i < remarks.size(); ++i)
    {
        fprintf(outfile, " REMARKS ");
        fprintf(outfile, "%s", remarks[i].c_str());
        fputc('\n', outfile);
    }
    fputc('\n', outfile);
}

void PSFOutput::PrintAtoms(FILE* outfile) const
{
    fprintf(outfile, headerFormat, totalAtoms, atomHeader);
    //silly psfs index from 1
    uint atomID = 1;
    uint molID = 1;
    for(uint mol = 0; mol < molecules->count; ++mol)
    {
        uint thisKind = molecules->kIndex[mol];
        uint nAtoms = molKinds[thisKind].atoms.size();
        for(uint at = 0; at < nAtoms; ++at)
        {
            const Atom* thisAtom = &molKinds[thisKind].atoms[at];
            //atom ID, segment name, residue ID, residue name, 
            //atom name, atom type, charge, mass, and an unused 0
            fprintf(outfile, atomFormat, atomID, molNames[thisKind].c_str(), molID, molNames[thisKind].c_str(),
                thisAtom->name.c_str(), thisAtom->type.c_str(), thisAtom->charge, thisAtom->mass, 0);
            ++atomID;
        }
        ++molID;
    }
    fputc('\n', outfile);
}

void PSFOutput::PrintBonds(FILE* outfile) const
{
    fprintf(outfile, headerFormat, totalBonds, bondHeader);
    uint atomID = 1;
     uint lineEntry = 0;
     for(uint mol = 0; mol < molecules->count; ++mol)
     {
        const MolKind& thisKind = molKinds[molecules->kIndex[mol]];
         for(uint i = 0; i < thisKind.bonds.size(); ++i)
         {
             fprintf(outfile, "%8d%8d", thisKind.bonds[i].a0 + atomID, 
                     thisKind.bonds[i].a1 + atomID);
             ++lineEntry;
             if(lineEntry == bondPerLine)
             {
                 lineEntry = 0;
                 fputc('\n', outfile);
             }
         }
         atomID += thisKind.atoms.size();
     }
     fputs("\n\n", outfile);
 }

 void PSFOutput::PrintAngles(FILE* outfile) const
 {
     fprintf(outfile, headerFormat, totalAngles, angleHeader);
     uint atomID = 1;
     uint lineEntry = 0;
     for(uint mol = 0; mol < molecules->count; ++mol)
     {
         const MolKind& thisKind = molKinds[molecules->kIndex[mol]];
         for(uint i = 0; i < thisKind.angles.size(); ++i)
         {
             fprintf(outfile, "%8d%8d%8d", thisKind.angles[i].a0 + atomID,
                     thisKind.angles[i].a1 + atomID, 
                     thisKind.angles[i].a2 + atomID);
             ++lineEntry;
             if(lineEntry == anglePerLine)
             {
                 lineEntry = 0;
                 fputc('\n', outfile);
             }
         }
         atomID += thisKind.atoms.size();
     }
     fputs("\n\n", outfile);
 }
 void PSFOutput::PrintDihedrals(FILE* outfile) const
 {
     fprintf(outfile, headerFormat, totalDihs, dihedralHeader);
     uint atomID = 1;
     uint lineEntry = 0;
     for(uint mol = 0; mol < molecules->count; ++mol)
     {
         const MolKind& thisKind = molKinds[molecules->kIndex[mol]];
         for(uint i = 0; i < thisKind.dihedrals.size(); ++i)
         {
            fprintf(outfile, "%8d%8d%8d%8d", thisKind.dihedrals[i].a0 + atomID,
                    thisKind.dihedrals[i].a1 + atomID,
                    thisKind.dihedrals[i].a2 + atomID,
                    thisKind.dihedrals[i].a3 + atomID);
             ++lineEntry;
             if(lineEntry == dihPerLine)
             {
                 lineEntry = 0;
                 fputc('\n', outfile);
             }
         }
         atomID += thisKind.atoms.size();
     }
     fputs("\n\n", outfile);
 }

