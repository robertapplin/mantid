#ifndef CONV2_MDEVENTS_COORD_TRANSF_TEST_H_
#define CONV2_MDEVENTS_COORD_TRANSF_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"
#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/Progress.h"
#include "MantidMDAlgorithms/ConvertToMDEventsCoordTransf.h"

//namespace ConvertToMDEventsTest
//{

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::Geometry;


class ConvertToMDEventsCoordTestHelper :public IConvertToMDEventsMethods
{
    size_t conversionChunk(size_t job_ID){UNUSED_ARG(job_ID);return 0;}
public:
    void setUPConversion(Mantid::API::MatrixWorkspace_sptr pWS2D, const PreprocessedDetectors &detLoc)
    {
        MDEvents::MDWSDescription TestWS(4);

        TestWS.Ei   = *(dynamic_cast<Kernel::PropertyWithValue<double>  *>(pWS2D->run().getProperty("Ei")));
        TestWS.emode= MDAlgorithms::Direct;
        TestWS.dim_min.assign(4,-3);
        TestWS.dim_max.assign(4,3);
        TestWS.dim_names[1]="phi";
        TestWS.dim_names[2]="chi";
        TestWS.dim_names[3]="omega";

        boost::shared_ptr<MDEvents::MDEventWSWrapper> pOutMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
        pOutMDWSWrapper->createEmptyMDWS(TestWS);

        IConvertToMDEventsMethods::setUPConversion(pWS2D,detLoc,TestWS,pOutMDWSWrapper);

    }
    void resetConversion(Mantid::API::MatrixWorkspace_sptr pWS2D, const PreprocessedDetectors &detLoc,const MDEvents::MDWSDescription &TestWS){

        boost::shared_ptr<MDEvents::MDEventWSWrapper> pOutMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
        pOutMDWSWrapper->createEmptyMDWS(TestWS);

        IConvertToMDEventsMethods::setUPConversion(pWS2D,detLoc,TestWS,pOutMDWSWrapper);

    }
    /// method which starts the conversion procedure
    void runConversion(API::Progress *){} 
};

//
class ConvertToMDEventsCoordTransfTest : public CxxTest::TestSuite, public ConvertToMDEvents
{
   Mantid::API::MatrixWorkspace_sptr ws2D;
//   static Mantid::Kernel::Logger &g_log;
   std::auto_ptr<API::Progress > pProg;
   std::auto_ptr<ConvertToMDEventsCoordTestHelper> pConvMethods;
   PreprocessedDetectors det_loc;

public:
static ConvertToMDEventsCoordTransfTest *createSuite() {
    return new ConvertToMDEventsCoordTransfTest(); 
    //g_log = Kernel::Logger::get("MD-Algorithms-Tests");
}
static void destroySuite(ConvertToMDEventsCoordTransfTest  * suite) { delete suite; }    

void test_CoordTransfNOQ()
{
    COORD_TRANSFORMER<NoQ,ANY_Mode,ConvertNo,Histohram> Copy;
    Copy.setUpTransf(pConvMethods.get());
    std::vector<coord_t> Coord(4);

    // copy one generic variable from WS axis (Y axis is not deined)
    TS_ASSERT_THROWS_NOTHING(Copy.calcGenericVariables(Coord,4));
    TS_ASSERT_THROWS_NOTHING(Copy.calcYDepCoordinates(Coord,0));
    const MantidVec& X        = ws2D->readX(0);
    size_t n_data = (X.size()-1);
    for(size_t i=0;i<n_data;i++){
        TS_ASSERT_THROWS_NOTHING(Copy.calcMatrixCoord(X,0,i,Coord));
        TS_ASSERT_DELTA(0.5*(X[i]+X[i+1]),Coord[0],1.e-5);  
    }
}

void test_CoordTransfQ3DDirect()
{
    COORD_TRANSFORMER<Q3D,Direct,ConvertNo,Histohram> ConvFromHisto;

    MDEvents::MDWSDescription TestWS(4);

    TestWS.Ei   = *(dynamic_cast<Kernel::PropertyWithValue<double>  *>(ws2D->run().getProperty("Ei")));
    TestWS.emode= MDAlgorithms::Direct;
    TestWS.dim_min.assign(4,-3);
    TestWS.dim_max.assign(4,3);
    TestWS.dim_names.assign(4,"Momentum");
    TestWS.dim_names[3]="DeltaE";
    TestWS.rotMatrix.assign(9,0);
    TestWS.rotMatrix[0]=1;
    TestWS.rotMatrix[4]=1;
    TestWS.rotMatrix[8]=1;


    pConvMethods->resetConversion(ws2D,det_loc,TestWS);

    const size_t specSize = ws2D->blocksize();
    size_t nValidSpectra = det_loc.nDetectors();

    // helper conversion to TOF
    UNITS_CONVERSION<ConvByTOF,Histohram> ConvToTOF;
    TS_ASSERT_THROWS_NOTHING(ConvToTOF.setUpConversion(pConvMethods.get(),"TOF"));

    // set up the run over the histohram methods
    ConvFromHisto.setUpTransf(pConvMethods.get());
    std::vector<coord_t> Coord(4);

    // copy one generic variable from WS axis (Y axis is not deined)
    TS_ASSERT_THROWS_NOTHING(ConvFromHisto.calcGenericVariables(Coord,4));    

    std::vector<coord_t> allCoordDir;
    allCoordDir.reserve(specSize*nValidSpectra*4);

    size_t ic(0);
    std::vector<double> TOF_data(specSize*nValidSpectra);

    for (size_t i = 0; i < nValidSpectra; ++i){
            size_t iSpctr             = det_loc.detIDMap[i];
            //int32_t det_id            = det_loc.det_id[i];

            const MantidVec& X        = ws2D->readX(iSpctr);        
            // calculate the coordinates which depend on detector posision 
            TS_ASSERT_THROWS_NOTHING(ConvFromHisto.calcYDepCoordinates(Coord,i));
            
            TS_ASSERT_THROWS_NOTHING(ConvToTOF.updateConversion(i));

         //=> START INTERNAL LOOP OVER THE "TIME"
            for (size_t j = 0; j < specSize; ++j)
            {
              TS_ASSERT_THROWS_NOTHING(ConvFromHisto.calcMatrixCoord(X,i,j,Coord));
              allCoordDir.insert(allCoordDir.end(),Coord.begin(),Coord.end());

              // convert to TOF for comparsion
              TS_ASSERT_THROWS_NOTHING(TOF_data[ic]=ConvToTOF.getXConverted(X,j));
              ic++;
            }   
    }
    // compare with conversion from TOF

    COORD_TRANSFORMER<Q3D,Direct,ConvFromTOF,Histohram> ConvFromTOFHisto;

    // make axis untit to be TOF to be able to work with conversion from TOF
    NumericAxis *pAxis0 = new NumericAxis(specSize); 
    pAxis0->setUnit("TOF");
    ws2D->replaceAxis(0,pAxis0);

    TS_ASSERT_THROWS_NOTHING(ConvFromTOFHisto.setUpTransf(pConvMethods.get()));    
    TS_ASSERT_THROWS_NOTHING(ConvFromTOFHisto.calcGenericVariables(Coord,4));    
    size_t icc(0);
    ic = 0;    for (size_t i = 0; i < nValidSpectra; ++i){
        //size_t iSpctr             = det_loc.detIDMap[i];
        //int32_t det_id            = det_loc.det_id[i];

        // calculate the coordinates which depend on detector posision 
         TS_ASSERT_THROWS_NOTHING(ConvFromTOFHisto.calcYDepCoordinates(Coord,i));
           
         //=> START INTERNAL LOOP OVER THE "TIME"
         for (size_t j = 0; j < specSize; ++j){

              TS_ASSERT_THROWS_NOTHING(ConvFromTOFHisto.ConvertAndCalcMatrixCoord(TOF_data[ic],Coord));
              // compare with results from TOF
              TS_ASSERT_DELTA(allCoordDir[icc+0],Coord[0],1.e-5);
              TS_ASSERT_DELTA(allCoordDir[icc+1],Coord[1],1.e-5);
              TS_ASSERT_DELTA(allCoordDir[icc+2],Coord[2],1.e-5);
              TS_ASSERT_DELTA(allCoordDir[icc+3],Coord[3],1.e-5);
              icc+=4;
              ic++;
            }   
    }


}



ConvertToMDEventsCoordTransfTest (){    

   std::vector<double> L2(5,5);
   std::vector<double> polar(5,(30./180.)*3.1415926);
   polar[0]=0;
   std::vector<double> azimutal(5,0);
   azimutal[1]=(45./180.)*3.1415936;
   azimutal[2]=(90./180.)*3.1415936;
   azimutal[3]=(135./180.)*3.1415936;
   azimutal[4]=(180./180.)*3.1415936;

   int numBins=10;
   ws2D =WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azimutal,numBins,-1,3,3);

   Kernel::UnitFactory::Instance().create("TOF");
   Kernel::UnitFactory::Instance().create("Energy");
   Kernel::UnitFactory::Instance().create("DeltaE");
   Kernel::UnitFactory::Instance().create("Momentum");

   // set up workpspaces and preprocess detectors
    pProg =  std::auto_ptr<API::Progress >(new API::Progress(dynamic_cast<ConvertToMDEvents *>(this),0.0,1.0,4));

    processDetectorsPositions(ws2D,det_loc,ConvertToMDEvents::getLogger(),pProg.get());
    pConvMethods = std::auto_ptr<ConvertToMDEventsCoordTestHelper>(new ConvertToMDEventsCoordTestHelper());
    pConvMethods->setUPConversion(ws2D,det_loc);


}

};

//} // end Namespace
#endif


