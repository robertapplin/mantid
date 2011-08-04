//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/TextAxis.h"

namespace 
{
  /// The percentage 'fuzziness' to use when comparing to bin boundaries
  const double xBoundaryTolerance = 1.0e-15;
}

namespace Mantid
{
namespace Algorithms
{

using std::size_t;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CropWorkspace)

/// Sets documentation strings for this algorithm
void CropWorkspace::initDocs()
{
  this->setWikiSummary("Extracts a 'block' from a workspace and places it in a new workspace. ");
  this->setOptionalMessage("Extracts a 'block' from a workspace and places it in a new workspace.");
}


using namespace Kernel;
using namespace API;

/// Default constructor
CropWorkspace::CropWorkspace() : 
  Algorithm(), m_minX(0), m_maxX(0), m_minSpec(-1), m_maxSpec(-1),
  m_commonBoundaries(false), m_histogram(false), m_croppingInX(false)
{}

/// Destructor
CropWorkspace::~CropWorkspace() {}

void CropWorkspace::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
    "The input Workspace2D" );
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "Name of the output workspace2D" );

  declareProperty("XMin", EMPTY_DBL(),
    "An X value that is within the first (lowest X value) bin that will be retained\n"
    "(default: workspace min)");
  declareProperty("XMax", EMPTY_DBL(),
    "An X value that is in the highest X value bin to be retained (default: max X)");
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex",0, mustBePositive,
    "The index number of the first entry in the Workspace that will be loaded\n"
    "(default: first entry in the Workspace)");
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive->clone(),
    "The index number of the last entry in the Workspace to be loaded\n"
    "(default: last entry in the Workspace)");
}

/** Executes the algorithm
 *  @throw std::out_of_range If a property is set to an invalid value for the input workspace
 */
void CropWorkspace::exec()
{
  // Get the input workspace
  m_inputWorkspace = getProperty("InputWorkspace");
  m_histogram = m_inputWorkspace->isHistogramData();
  // Check for common boundaries in input workspace
  m_commonBoundaries = WorkspaceHelpers::commonBoundaries(m_inputWorkspace);

  // Retrieve and validate the input properties
  this->checkProperties();

  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace =
    WorkspaceFactory::Instance().create(m_inputWorkspace,m_maxSpec-m_minSpec+1,m_maxX-m_minX,m_maxX-m_minX-m_histogram);

  // If this is a Workspace2D, get the spectra axes for copying in the spectraNo later
  Axis *inAxis1(NULL), *outAxis1(NULL);
  TextAxis *outTxtAxis(NULL);
  if (m_inputWorkspace->axes() > 1)
  {
    inAxis1 = m_inputWorkspace->getAxis(1);
    outAxis1 = outputWorkspace->getAxis(1);
    outTxtAxis = dynamic_cast<TextAxis*>(outAxis1);
  }

  cow_ptr<MantidVec> newX;
  if ( m_commonBoundaries )
  {
    const MantidVec& oldX = m_inputWorkspace->readX(m_minSpec);
    newX.access().assign(oldX.begin()+m_minX,oldX.begin()+m_maxX);
  }
  Progress prog(this,0.0,1.0,(m_maxSpec-m_minSpec));
  // Loop over the required spectra, copying in the desired bins
  for (int i = m_minSpec, j = 0; i <= m_maxSpec; ++i,++j)
  {
    // Preserve/restore sharing if X vectors are the same
    if ( m_commonBoundaries )
    {
      outputWorkspace->setX(j,newX);
    }
    else
    {
      // Safe to just copy whole vector 'cos can't be cropping in X if not common
      outputWorkspace->setX(j,m_inputWorkspace->refX(i));
    }

    const MantidVec& oldY = m_inputWorkspace->readY(i);
    outputWorkspace->dataY(j).assign(oldY.begin()+m_minX,oldY.begin()+(m_maxX-m_histogram));
    const MantidVec& oldE = m_inputWorkspace->readE(i);
    outputWorkspace->dataE(j).assign(oldE.begin()+m_minX,oldE.begin()+(m_maxX-m_histogram));
    
    //copy over the axis entry for each spectrum, regardless of the type of axes present
    if (inAxis1) 
    {
      if( outAxis1->isText() )
      {
        outTxtAxis->setLabel(j, inAxis1->label(i));
      }
      else
      {
        outAxis1->setValue(j, inAxis1->operator()(i));
      }
    }

    if ( !m_commonBoundaries ) this->cropRagged(outputWorkspace,i,j);
    
    // Propagate bin masking if there is any
    if ( m_inputWorkspace->hasMaskedBins(i) )
    {
      const MatrixWorkspace::MaskList& inputMasks = m_inputWorkspace->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it)
      {
        const size_t maskIndex = (*it).first;
        if ( maskIndex >= m_minX && maskIndex < m_maxX-m_histogram )
          outputWorkspace->flagMasked(j,maskIndex-m_minX,(*it).second);
      }
    }
    prog.report();
  }
  if( inAxis1->isSpectra() )
  {
    outputWorkspace->updateSpectraUsingMap();
  }

  setProperty("OutputWorkspace", outputWorkspace);
}

/** Retrieves the optional input properties and checks that they have valid values.
 *  Assigns to the defaults if any property has not been set.
 *  @throw std::invalid_argument If the input workspace does not have common binning
 *  @throw std::out_of_range If a property is set to an invalid value for the input workspace
 */
void CropWorkspace::checkProperties()
{
  m_minX = this->getXMin();
  m_maxX = this->getXMax();
  const size_t xSize = m_inputWorkspace->readX(0).size();
  if ( m_minX > 0 || m_maxX < xSize )
  {
    if ( m_minX > m_maxX )
    {
      g_log.error("XMin must be less than XMax");
      throw std::out_of_range("XMin must be less than XMax");
    }
    if ( m_minX == m_maxX && m_commonBoundaries )
    {
      g_log.error("The X range given lies entirely within a single bin");
      throw std::out_of_range("The X range given lies entirely within a single bin");
    }
    m_croppingInX = true;
  }
  if ( !m_commonBoundaries ) m_minX = 0;
  if ( !m_commonBoundaries ) m_maxX = static_cast<int>(m_inputWorkspace->readX(0).size());

  m_minSpec = getProperty("StartWorkspaceIndex");
  const int numberOfSpectra = static_cast<int>(m_inputWorkspace->getNumberHistograms());
  m_maxSpec = getProperty("EndWorkspaceIndex");
  if ( isEmpty(m_maxSpec) ) m_maxSpec = numberOfSpectra-1;

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if ( m_minSpec > numberOfSpectra-1 )
  {
    g_log.error("StartWorkspaceIndex out of range!");
    throw std::out_of_range("StartSpectrum out of range!");
  }
  if ( m_maxSpec > numberOfSpectra-1 )
  {
    g_log.error("EndWorkspaceIndex out of range!");
    throw std::out_of_range("EndWorkspaceIndex out of range!");
  }
  if ( m_maxSpec < m_minSpec )
  {
    g_log.error("StartWorkspaceIndex must be less than or equal to EndWorkspaceIndex");
    throw std::out_of_range("StartWorkspaceIndex must be less than or equal to EndWorkspaceIndex");
  }
}

/** Find the X index corresponding to (or just within) the value given in the XMin property.
 *  Sets the default if the property has not been set.
 *  @param  wsIndex The workspace index to check (default 0).
 *  @return The X index corresponding to the XMin value.
 */
size_t CropWorkspace::getXMin(const int wsIndex)
{
  double minX_val = getProperty("XMin");
  size_t xIndex = 0;
  if ( ! isEmpty(minX_val) )
  {//A value has been passed to the algorithm, check it and maybe store it
    const MantidVec& X = m_inputWorkspace->readX(wsIndex);
    if ( m_commonBoundaries && minX_val > X.back() )
    {
      g_log.error("XMin is greater than the largest X value");
      throw std::out_of_range("XMin is greater than the largest X value");
    }
    // Reduce cut-off value slightly to allow for rounding errors
    // when trying to exactly hit a bin boundary.
    minX_val -= std::abs(minX_val*xBoundaryTolerance);
    xIndex = std::lower_bound(X.begin(),X.end(),minX_val) - X.begin();
  }
  return xIndex;
}

/** Find the X index corresponding to (or just within) the value given in the XMax property.
 *  Sets the default if the property has not been set.
 *  @param  wsIndex The workspace index to check (default 0).
 *  @return The X index corresponding to the XMax value.
 */
size_t CropWorkspace::getXMax(const int wsIndex)
{
  const MantidVec& X = m_inputWorkspace->readX(wsIndex);
  size_t xIndex = X.size();
  //get the value that the user entered if they entered one at all
  double maxX_val = getProperty("XMax");
  if ( ! isEmpty(maxX_val) )
  {//we have a user value, check it and maybe store it
    if ( m_commonBoundaries && maxX_val < X.front() )
    {
      g_log.error("XMax is less than the smallest X value");
      throw std::out_of_range("XMax is less than the smallest X value");
    }
    // Increase cut-off value slightly to allow for rounding errors
    // when trying to exactly hit a bin boundary.
    maxX_val += std::abs(maxX_val*xBoundaryTolerance);
    xIndex = std::upper_bound(X.begin(),X.end(),maxX_val) - X.begin();
  }
  return xIndex;
}

/** Zeroes all data points outside the X values given
 *  @param outputWorkspace :: The output workspace - data has already been copied
 *  @param inIndex ::         The workspace index of the spectrum in the input workspace
 *  @param outIndex ::        The workspace index of the spectrum in the output workspace
 */
void CropWorkspace::cropRagged(API::MatrixWorkspace_sptr outputWorkspace, int inIndex, int outIndex)
{
  MantidVec& Y = outputWorkspace->dataY(outIndex);
  MantidVec& E = outputWorkspace->dataE(outIndex);
  const size_t size = Y.size();
  size_t startX = this->getXMin(inIndex);
  if (startX > size) startX = size;
  for (size_t i = 0; i < startX; ++i)
  {
    Y[i] = 0.0;
    E[i] = 0.0;
  }
  size_t endX = this->getXMax(inIndex);
  if ( endX > 0 ) endX -= m_histogram;
  for (size_t i = endX; i < size; ++i)
  {
    Y[i] = 0.0;
    E[i] = 0.0;
  }
}

} // namespace Algorithms
} // namespace Mantid
