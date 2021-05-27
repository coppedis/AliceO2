// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef DETECTOR_ZDCDCSPROCESSOR_H_
#define DETECTOR_ZDCDCSPROCESSOR_H_

#include <memory>
#include <Rtypes.h>
#include <unordered_map>
#include <deque>
#include <numeric>
#include "Framework/Logger.h"
#include "DetectorsDCS/DataPointCompositeObject.h"
#include "DetectorsDCS/DataPointIdentifier.h"
#include "DetectorsDCS/DataPointValue.h"
#include "DetectorsDCS/DeliveryType.h"
#include "CCDB/CcdbObjectInfo.h"
#include "CommonUtils/MemFileHelper.h"
#include "CCDB/CcdbApi.h"
#include <gsl/gsl>
#include "ZDCBase/ModuleConfig.h"
#include "ZDCBase/Constants.h"

/// @brief Class to process DCS data points

namespace o2
{
namespace zdc
{

using DPID = o2::dcs::DataPointIdentifier;
using DPVAL = o2::dcs::DataPointValue;
using DPCOM = o2::dcs::DataPointCompositeObject;

struct ZDCDCSinfo {
  std::pair<uint64_t, double> firstValue; // first value seen by the ZDC DCS processor
  std::pair<uint64_t, double> lastValue;  // last value seen by the ZDC DCS processor
  std::pair<uint64_t, double> midValue;   // mid value seen by the TOF DCS processor
  std::pair<uint64_t, double> maxChange;  // maximum variation seen by the TOF DCS processor

  ZDCDCSinfo()
  {
    firstValue = std::make_pair(0, -999999999);
    lastValue = std::make_pair(0, -999999999);
    midValue = std::make_pair(0, -999999999);
    maxChange = std::make_pair(0, -999999999);
  }
  void makeEmpty()
  {
    firstValue.first = lastValue.first = midValue.first = maxChange.first = 0;
    firstValue.second = lastValue.second = midValue.second = maxChange.second = -999999999;
  }
  void print() const;

  ClassDefNV(ZDCDCSinfo, 1);
};

struct ZDCModuleMap{
  std::array<int8_t, 4> moduleID = {-1, -1, -1, -1};
  std::array<bool, 4> readChannel = {false, false, false, false};
  std::array<bool, 4> triggerChannel = {false, false, false, false};
  //std::array<TriggerChannelConfig, 4> trigChannelConf;
};

/*struct TriggerChannelConfig {
  int8_t id = -1;
  int8_t first = 0;
  int8_t last = 0;
  uint8_t shift = 0;
  int16_t threshold = 0;
};*/

class ZDCDCSProcessor
{

 public:
  using TFType = uint64_t;
  using CcdbObjectInfo = o2::ccdb::CcdbObjectInfo;
  using DQDoubles = std::deque<double>;

  static constexpr int NDDLS = 16;
  static constexpr int NModules = 8;
  static constexpr int NChannels = 4;
  static constexpr int NHVChannels = 22+12; //no. of HV+additional HV channels

  ZDCDCSProcessor() = default;
  ~ZDCDCSProcessor() = default;

  void init(const std::vector<DPID>& pids);

  int process(const gsl::span<const DPCOM> dps);
  int processDP(const DPCOM& dpcom);
  virtual uint64_t processFlags(uint64_t flag, const char* pid);

  void updateDPsCCDB();
  void getZDCActiveChannels(int nDDL, int nModule, ZDCModuleMap& info) const;
  void updateMappingCCDB();
  void updateHVCCDB();
  void updatePositionCCDB();

  const CcdbObjectInfo& getccdbDPsInfo() const { return mccdbDPsInfo; }
  CcdbObjectInfo& getccdbDPsInfo() { return mccdbDPsInfo; }
  const std::unordered_map<DPID, ZDCDCSinfo>& getZDCDPsInfo() const { return mZDCDCS; }

  const CcdbObjectInfo& getMappingStatus() const { return mZDCMapInfo; }
  CcdbObjectInfo& getMappingStatus() const { return mZDCMapInfo; }
  const std::bitset<NChannels>& getMappingStatus() const { return mMapping; }
  bool isMappingUpdated() const { return mUpdateMapping; }

  const CcdbObjectInfo& getccdbHVInfo() const { return mccdbHVInfo; }
  CcdbObjectInfo& getccdbHVInfo() { return mccdbHVInfo; }
  const std::bitset<NChannels>& getHVStatus() const { return mHV; }
  bool isHVUpdated() const { return mUpdateHVStatus; }

  const CcdbObjectInfo& getccdbPositionInfo() const { return mccdbPositionInfo; }
  CcdbObjectInfo& getccdbPositionInfo() { return mccdbPositionInfo; }
  const std::bitset<4>& getVerticalPosition() const { return mVerticalPosition; }
  bool isPositionUpdated() const { return mVerticalPositionStatus; }

  template <typename T>
  void prepareCCDBobjectInfo(T& obj, CcdbObjectInfo& info, const std::string& path, TFType tf,
                             const std::map<std::string, std::string>& md);

  void setTF(TFType tf) { mTF = tf; }
  void useVerboseMode() { mVerbose = true; }

  void clearDPsinfo()
  {
    mDpsdoublesmap.clear();
    mZDCDCS.clear();
  }

 private:
  std::unordered_map<DPID, ZDCDCSinfo> mZDCDCS; // object that will go to the CCDB
  std::unordered_map<DPID, bool> mPids;         // contains all PIDs for the processor
                                                // bool is true if the DP was processed at least once
  std::unordered_map<DPID, std::vector<DPVAL>> mDpsdoublesmap; // map that will hold the DPs

  std::array<std::array<ZDCModuleMap, NModules>, NDDLS> mMapInfo; // contains the mapping info...
  std::array<std::bitset<4>, NDDLS> mPreviousMapping;             // previous mapping
  std::bitset<NChannels> mMapping;                                // bitset with status per channel
  bool mUpdateMapping = false;                                    // whether to update mapping in CCDB

  std::bitset<NHVChannels> mHV;           // bitset with HV status per channel
  std::bitset<NHVChannels> mPrevHVstatus; // previous HV status
  bool mUpdateHVStatus = false;           // whether to update the HV status in CCDB

  std::bitset<4> mVerticalPosition;      // bitset with hadronic calorimeter position
  std::bitset<4> mPrevPositionstatus;    // previous position values
  bool mUpdateVerticalPosition = false; // whether to update the hadronic calorimeter position

  CcdbObjectInfo mccdbDPsInfo;
  CcdbObjectInfo mZDCMapInfo;
  CcdbObjectInfo mccdbHVInfo;
  CcdbObjectInfo mccdbPositionInfo;
  TFType mStartTF; // TF index for processing of first processed TF, used to store CCDB object
  TFType mTF = 0;  // TF index for processing, used to store CCDB object
  bool mStartTFset = false;

  bool mVerbose = false;

  ClassDefNV(ZDCDCSProcessor, 0);
};

template <typename T>
void ZDCDCSProcessor::prepareCCDBobjectInfo(T& obj, CcdbObjectInfo& info, const std::string& path, TFType tf,
                                            const std::map<std::string, std::string>& md)
{

  // prepare all info to be sent to CCDB for object obj
  auto clName = o2::utils::MemFileHelper::getClassName(obj);
  auto flName = o2::ccdb::CcdbApi::generateFileName(clName);
  info.setPath(path);
  info.setObjectType(clName);
  info.setFileName(flName);
  info.setStartValidityTimestamp(tf);
  info.setEndValidityTimestamp(99999999999999);
  info.setMetaData(md);
}

} // namespace ZDC
} // namespace o2

#endif
