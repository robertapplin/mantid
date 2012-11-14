/*WIKI*
This algorithm is intended to append the geometry information into a raw NeXus file.
It is initially for use only at the SNS, as it is needed for the currently upgrade program.
But there is nothing preventing it being used elsewhere.

The algorithm takes the geometry information in the IDF togther with the log values in a given NeXus file
and calculates the resolved positions of all the detectors and then writes this into the NeXus file specified.

*WIKI*/

#include "MantidDataHandling/AppendGeometryToSNSNexus.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidNexusCPP/NeXusException.hpp"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Exception.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace ::NeXus;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(AppendGeometryToSNSNexus)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  AppendGeometryToSNSNexus::AppendGeometryToSNSNexus()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  AppendGeometryToSNSNexus::~AppendGeometryToSNSNexus()
  {
    //delete workspace
  }
  
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string AppendGeometryToSNSNexus::name() const { return "AppendGeometryToSNSNexus";}
  
  /// Algorithm's version for identification. @see Algorithm::version
  int AppendGeometryToSNSNexus::version() const { return 1;}
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string AppendGeometryToSNSNexus::category() const { return "DataHandling\\DataAcquisition";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void AppendGeometryToSNSNexus::initDocs()
  {
    this->setWikiSummary("Appends the resolved instrument geometry (detectors and monitors for now) to a SNS ADARA NeXus file.");
    this->setOptionalMessage("Appends the resolved instrument geometry (detectors and monitors for now) to a SNS ADARA NeXus file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void AppendGeometryToSNSNexus::init()
  {
      // Declare potential extensions for input NeXus file
      std::vector<std::string> extensions;
      extensions.push_back(".nxs");
      extensions.push_back(".h5");

      declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, extensions),
                      "The name of the NeXus file to append geometry to.");

      // TODO: change MakeCopy default to False when comfortable. Otherwise need to remove the extra copy once in production.
      declareProperty(new PropertyWithValue<bool>("MakeCopy", true, Direction::Input),
                      "Copy the NeXus file first before appending (optional, default True).");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void AppendGeometryToSNSNexus::exec()
  {          
      // TODO: rename the created arrays before moving to production
      g_log.warning() << "This is intended as a proof of principle and not a long term implementation." << std::endl;
      g_log.warning() << "(the created arrays in the NeXus file will have the '_new' suffix)" << std::endl;

      // Retrieve filename from the properties
      m_filename = getPropertyValue("Filename");

      // Are we going to make a copy of the file ?
      m_makeNexusCopy = getProperty("MakeCopy");

      if (m_makeNexusCopy)
      {
          Poco::File originalFile(m_filename);
          Poco::Path originalPath(m_filename);

          if (originalFile.exists())
          {
              Poco::File destinationFile(Poco::Path(Poco::Path::temp(), originalPath.getFileName()));

              try
              {
                  originalFile.copyTo(destinationFile.path());
                  g_log.notice() << "Copied " << m_filename << " to " << destinationFile.path() << "." << std::endl ;
                  m_filename = destinationFile.path();
              }
              catch (Poco::FileAccessDeniedException &)
              {
                  throw std::runtime_error("A Problem occurred in making a copy of the NeXus file. Failed to copy "
                                           + originalFile.path() + " to " + destinationFile.path()
                                           + ". Please check file permissions.");
              }
          }
          else
          {
              g_log.error() << "Cannot copy a file that doesn't exist! (" << originalFile.path() << ")." << std::endl;
          }

      }

      // Let's check to see if we can write to the NeXus file.
      if (!(Poco::File(m_filename).canWrite()))
      {
          throw std::runtime_error("The specified NeXus file (" + m_filename + ") is not writable.");
      }


      // Let's look for the instrument name
      m_instrument = getInstrumentName(m_filename);

      if (m_instrument.length() == 0)
      {

          throw std::runtime_error("Failed to get instrument name from " + m_filename + ". Can't identify instrument definition file.");
      }

      // Temp workspace name to load the instrument into
      //std::string workspaceName = "__" + m_instrument + "_geometry_ws";

      // Now what is the instrument definition filename ?
      // TODO: Modify to use /entry/instrument/instrument_xml/data after establishing a way to maintain ADARA Geometry Packet
      m_idf_filename = ExperimentInfo::getInstrumentFilename(m_instrument);
      g_log.debug() << "Loading instrument definition from " << m_idf_filename << "." << std::endl;

      // Let's load the empty instrument
      //IAlgorithm_sptr alg = AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
      //alg->initialize();
      //alg->setPropertyValue("Filename", m_idf_filename);
      //alg->setPropertyValue("OutputWorkspace", workspaceName);
      //alg->execute();

      //MatrixWorkspace_sptr ws;
      //ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);

      // Modified to call LoadInstrument directly as a sub-algorithm
      ws = WorkspaceFactory::Instance().create("Workspace2D",1,2,1);

      // Load NeXus logs for HYSPEC, HYSPECA(testing), and SNAP
      if(m_instrument == "HYSPEC" || m_instrument == "HYSPECA" || m_instrument == "SNAP")
      {
          g_log.debug() << "Run LoadNexusLogs sub-algorithm." << std::endl;
          logs_loaded_correctly = runLoadNexusLogs(m_filename, ws, this);

          if(!logs_loaded_correctly)
              throw std::runtime_error("Failed to run LoadNexusLogs sub-algorithm.");
      }

      g_log.debug() << "Run LoadInstrument sub-algorithm." << std::endl;
      instrument_loaded_correctly = runLoadInstrument(m_idf_filename, ws, this);

      if(!instrument_loaded_correctly)
          throw std::runtime_error("Failed to run LoadInstrument sub-algorithm.");

      // Get the number of detectors (just for progress reporting)
      // Get the number of histograms/detectors
      const size_t numDetectors = ws->getInstrument()->getDetectorIDs().size();

      this->progress = new API::Progress(this, 0.0, 1.0, numDetectors);


      // Get the instrument
      Geometry::Instrument_const_sptr instrument = ws->getInstrument();

      // Get the sample (needed to calculate distances)
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      // Get the source (moderator)
      Geometry::IObjComponent_const_sptr source = instrument->getSource();

      // Open the NeXus file
      ::NeXus::File nxfile(m_filename, NXACC_RDWR);


      //typedef std::map<std::string,std::string> string_map_t;
      std::map<std::string,std::string>::const_iterator root_iter;
      std::map<std::string, std::string> entries = nxfile.getEntries();

      for (root_iter = entries.begin(); root_iter != entries.end(); ++root_iter)
      {
          // Open all NXentry
          if (root_iter->second == "NXentry")
          {
              nxfile.openGroup(root_iter->first, "NXentry");

              // Get a list of items within the entry.
              std::map<std::string, std::string> entry_items = nxfile.getEntries();
              // Create an iterator for this
              std::map<std::string,std::string>::const_iterator entry_iter;


              for (entry_iter = entry_items.begin(); entry_iter != entry_items.end(); ++entry_iter)
              {
                  // Look for an instrument
                  if (entry_iter->second == "NXinstrument")
                  {
                      // Open the instrument
                      nxfile.openGroup(entry_iter->first, "NXinstrument");
                      std::map<std::string, std::string> instr_items = nxfile.getEntries();
                      std::map<std::string,std::string>::const_iterator instr_iter;
                      for (instr_iter = instr_items.begin(); instr_iter != instr_items.end(); ++instr_iter)
                      {
                          // Look for NXdetectors
                          if (instr_iter->second == "NXdetector")
                          {
                              g_log.debug() << "Detector called '" << instr_iter->first << "' found." << std::endl;
                              std::string bankName = instr_iter->first;
                              std::vector<Geometry::IDetector_const_sptr> dets;
                              ws->getInstrument()->getDetectorsInBank(dets, bankName);

                              if (!dets.empty())
                              {
                                  nxfile.openGroup(bankName, "NXdetector");

                                  // Let's create some vectors for the parameters to write
                                  // Pixel IDs
                                  std::vector<int> pixel_id;
                                  std::vector<double> distance;
                                  std::vector<double> polar_angle;
                                  std::vector<double> azimuthal_angle;

                                  pixel_id.reserve(dets.size());
                                  distance.reserve(dets.size());
                                  polar_angle.reserve(dets.size());
                                  azimuthal_angle.reserve(dets.size());

                                  for (std::size_t i=0; i < dets.size(); i++)
                                  {
                                      pixel_id.push_back(dets[i]->getID());
                                      distance.push_back(dets[i]->getDistance(*sample));
                                      azimuthal_angle.push_back(dets[i]->getPhi());
                                      polar_angle.push_back(ws->detectorTwoTheta(dets[i]));
                                  }

                                  // Write Pixel ID to file
                                  nxfile.writeData("pixel_id_new", pixel_id);

                                  // Write Secondary Flight Path to file
                                  nxfile.writeData("distance_new", distance);
                                  nxfile.openData("distance_new");
                                  nxfile.putAttr("units", "metre");
                                  nxfile.closeData();

                                  // Write Polar Angle (2theta) to file
                                  nxfile.writeData("polar_angle_new", polar_angle);
                                  nxfile.openData("polar_angle_new");
                                  nxfile.putAttr("units", "radian");
                                  nxfile.closeData();

                                  // Write Azimuthal Angle (Phi) to file
                                  nxfile.writeData("azimuthal_angle_new", azimuthal_angle);
                                  nxfile.openData("azimuthal_angle_new");
                                  nxfile.putAttr("units", "radian");
                                  nxfile.closeData();

                                  nxfile.closeGroup(); // close NXdetector

                                  this->progress->report(dets.size());
                              }
                              else
                              {
                                  throw std::runtime_error("Could not find any detectors for the bank named " + bankName +
                                                           " that is listed in the NeXus file."
                                                           "Check that it exists in the Instrument Definition File.");
                              }

                          }
                      }

                      nxfile.closeGroup(); // NXinstrument

                  }
                  // Look for monitors
                  else if (entry_iter->second == "NXmonitor")
                  {
                        g_log.debug() << "Monitor called '" << entry_iter->first << "' found." << std::endl;
                        nxfile.openGroup(entry_iter->first, "NXmonitor");

                        Geometry::IComponent_const_sptr monitor = instrument->getComponentByName(entry_iter->first);

                        // Write Pixel ID to file
                        //nxfile.writeData("pixel_id_new", monitor->get);

                        double source_monitor = source->getDistance(*monitor);
                        double source_sample = source->getDistance(*sample);

                        g_log.debug() << "source->monitor=" << source_monitor << std::endl;
                        g_log.debug() << "source->sample=" << source_sample << std::endl;
                        g_log.debug() << "sample->monitor=" << (source_monitor-source_sample) << std::endl;

                        // Distance
                        nxfile.writeData("distance_new", (source_monitor-source_sample));
                        nxfile.openData("distance_new");
                        nxfile.putAttr("units", "metre");
                        nxfile.closeData();


                        nxfile.closeGroup();  // NXmonitor

                  }


              }

          }
          else
          {
              g_log.error() << "There are no NXentry nodes in the specified NeXus file." << std::endl;
          }

      }


      // Clean up the workspace
      //AnalysisDataService::Instance().remove(workspaceName);
  }


  //----------------------------------------------------------------------------------------------
  /** Get the instrument name from the input NeXus file.
   *
   * @param nxfilename :: Input NeXus file.
   * @param the instrument name, empty string if failed.
   */
 std::string AppendGeometryToSNSNexus::getInstrumentName(const std::string &nxfilename)
 {
     std::string instrument;

     // Open the NeXus file
     ::NeXus::File nxfile(nxfilename);
     // What is the first entry ?
     std::map<std::string, std::string> entries = nxfile.getEntries();

     // For now, let's just open the first entry
     nxfile.openGroup(entries.begin()->first, "NXentry");
     g_log.debug() << "Using entry '" << entries.begin()->first << "' to determine instrument name." << std::endl;

     nxfile.openGroup("instrument", "NXinstrument");
     try
     {
         nxfile.openData("name");
         instrument = nxfile.getStrData();
     }
     catch (::NeXus::Exception &)
     {
         // TODO: try and get the instrument name from the filename instead.
         // Note in filename we have instrument short name yet ExperimentiInfo.getInstrumentFilename() expects instrument long name
         instrument = "";
     }

     g_log.debug() << " Instrument name read from NeXus file is " << instrument << std::endl;

     return instrument;
 }

  //----------------------------------------------------------------------------------------------
  /** Load the instrument using the input instrument definition file.
   *
   * @param idf_filename :: Input instrument definition file.
   * @param localWorkspace :: MatrixWorkspace in which to put the instrument geometry
   * @param alg :: Handle of an algorithm for logging access
   * @return true if successful
   */

  bool AppendGeometryToSNSNexus::runLoadInstrument(const std::string &idf_filename,
                                                   API::MatrixWorkspace_sptr localWorkspace, Algorithm * alg)
  {
    IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument",0,1,true);

    // Execute the sub-algorithm.
    bool executionSuccessful(true);
    try
    {
      loadInst->setPropertyValue("Filename", idf_filename);
      loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
      loadInst->setProperty("RewriteSpectraMap", false);
      loadInst->execute();
    } catch (std::invalid_argument& e)
    {
      alg->getLogger().information("Invalid argument to LoadInstrument sub-algorithm");
      alg->getLogger().information(e.what());
      executionSuccessful = false;
    } catch (std::runtime_error& e)
    {
      alg->getLogger().information("Failed to run LoadInstrument sub-algorithm");
      alg->getLogger().information(e.what());
      executionSuccessful = false;
    }

    // Throwing an error if failed
    if (!executionSuccessful)
    {
      alg->getLogger().error("Error loading instrument\n");
    }
    return executionSuccessful;
  }

  //-----------------------------------------------------------------------------
  /** Load the logs from the input NeXus file.
   *
   * @param nexusFileName :: Name of the NeXus file to load logs from.
   * @param localWorkspace :: MatrixWorkspace in which to put the logs.
   * @param alg :: Handle of an algorithm for logging access.
   * @return true if successful.
   */
   bool AppendGeometryToSNSNexus::runLoadNexusLogs(const std::string &nexusFileName,
                                                   API::MatrixWorkspace_sptr localWorkspace, Algorithm * alg)
   {
       IAlgorithm_sptr loadLogs = alg->createSubAlgorithm("LoadNexusLogs",0,1,true);

       // Execute the sub-algorithm, catching errors without stopping.
       bool executionSuccessful(true);
       try
       {
         alg->getLogger().information() << "Loading logs from the NeXus file..." << std::endl;
         loadLogs->setPropertyValue("Filename", nexusFileName);
         loadLogs->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
         loadLogs->executeAsSubAlg();
       } catch (std::invalid_argument& e)
       {
         alg->getLogger().information("Invalid argument to LoadNexusLogs sub-algorithm");
         alg->getLogger().information(e.what());
         executionSuccessful = false;
       } catch (std::runtime_error& )
       {
         alg->getLogger().information("Unable to successfully run runLoadNexusLogs sub-algorithm./n");
         executionSuccessful = false;
       }

       return executionSuccessful;
   }
} // namespace Mantid
} // namespace DataHandling
