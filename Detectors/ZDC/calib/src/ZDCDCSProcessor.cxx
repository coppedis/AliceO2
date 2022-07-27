  // Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "Rtypes.h"
#include <deque>
#include <string>
#include <algorithm>
#include <iterator>
#include <string>
#include <cstring>
#include <bitset>
#include <ZDCCalib/ZDCDCSProcessor.h>

using namespace o2::zdc;
using namespace o2::dcs;

using DeliveryType = o2::dcs::DeliveryType;
using DPID = o2::dcs::DataPointIdentifier;
using DPVAL = o2::dcs::DataPointValue;

ClassImp(o2::zdc::ZDCDCSinfo);

void ZDCDCSinfo::print() const
{
  LOG(INFO) << "First Value: timestamp = " << firstValue.first << ", value = " << firstValue.second;
  LOG(INFO) << "Last Value:  timestamp = " << lastValue.first << ", value = " << lastValue.second;
  LOG(INFO) << "Mid Value:   timestamp = " << midValue.first << ", value = " << midValue.second;
  LOG(INFO) << "Max Change:  timestamp = " << maxChange.first << ", value = " << maxChange.second;
}

//__________________________________________________________________

void ZDCDCSProcessor::init(const std::vector<DPID>& pids)
{
  // fill the array of the DPIDs that will be used by ZDC
  // pids should be provided by CCDB

  for (const auto& it : pids) {
    mPids[it] = false;
    mZDCDCS[it].makeEmpty();
  }

  for (int iddl = 0; iddl < NDDLS; ++iddl) {
    for (int im = 0; im < NMODULES; ++im) {
      getZDCActiveChannels(iddl, im, mZDCMapInfo[iddl][im]);
    }
  }
}

//__________________________________________________________________

int ZDCDCSProcessor::process(const gsl::span<const DPCOM> dps)
{

  // first we check which DPs are missing
  if (mVerbose) {
    LOG(INFO) << "\n\nProcessing new TF\n-----------------";
  }
  if (!mStartTFset) {
    mStartTF = mTF;
    mStartTFset = true;
  }

  std::unordered_map<DPID, DPVAL> mapin;
  for (auto& it : dps) {
    mapin[it.id] = it.data;
  }
  for (auto& it : mPids) {
    const auto& el = mapin.find(it.first);
    if (el == mapin.end()) {
      LOG(DEBUG) << "DP " << it.first << " not found in map";
    } else {
      LOG(DEBUG) << "DP " << it.first << " found in map";
    }
  }

  mUpdateMapping = false;          // by default no new entry in the CCDB for the mapping
  mUpdateHVStatus = false;         // by default no new entry in the CCDB for the HV
  mUpdateVerticalPosition = false; // by default no new entry in the CCDB for ZDC positions

  // now we process all DPs, one by one
  for (const auto& it : dps) {
    // we process only the DPs defined in the configuration
    const auto& el = mPids.find(it.id);
    if (el == mPids.end()) {
      LOG(INFO) << "DP " << it.id << " not found in ZDCDCSProcessor, will not process it";
      continue;
    }
    processDP(it);
    mPids[it.id] = true;
  }

  if (mUpdateMapping) {
    updateMappingCCDB();
  }

  if (mUpdateHVStatus) {
    updateHVCCDB();
  }

  if (mUpdateVerticalPosition) {
    updatePositionCCDB();
  }

  return 0;
}

//__________________________________________________________________

int ZDCDCSProcessor::processDP(const DPCOM& dpcom)
{

  // processing single DP

  auto& dpid = dpcom.id;
  const auto& type = dpid.get_type();
  auto& val = dpcom.data;
  if (mVerbose) {
    if (type == RAW_DOUBLE) { // positions, mapping and HV
      LOG(INFO);
      LOG(INFO) << "Processing DP " << dpcom << ", with value = " << o2::dcs::getValue<double>(dpcom);
    } else if (type == RAW_INT) { // mapping
      LOG(INFO);
      LOG(INFO) << "Processing DP " << dpcom << ", with value = " << o2::dcs::getValue<int32_t>(dpcom);
    }
  }
  auto flags = val.get_flags();
  if (processFlags(flags, dpid.get_alias()) == 0) {
    // access the correct element
    if (type == RAW_DOUBLE) { // full map & positions
      // for these DPs we store the first values
      /*auto& dvect = mDpsdoublesmap[dpid];
      LOG(DEBUG) << "mDpsdoublesmap[dpid].size() = " << dvect.size();
      auto etime = val.get_epoch_time();
      if (dvect.size() == 0 ||
          etime != dvect.back().get_epoch_time()) { // check timestamp (TBF)
          dvect.push_back(val);
      }*/

      if (std::strstr(dpid.get_alias(), "position") != nullptr) { // DP from POSITION
        std::bitset<4> posstatus(o2::dcs::getValue<double>(dpcom));
        if (mVerbose) {
          LOG(INFO) << " Prev.positions : " << mPrevPositionstatus << ", new = " << posstatus;
        }
        if (posstatus == mPrevPositionstatus) {
          if (mVerbose) {
            LOG(INFO) << "ZN/ZP positions unchanged, doing nothing";
          }
          return 0;
        }
        if (mVerbose) {
          LOG(INFO) << "Positions modified";
        }
        mUpdateVerticalPosition = true;
        mPrevPositionstatus = posstatus;
      }

      if (std::strstr(dpid.get_alias(), "HV") != nullptr) { // DP is HV value
        std::string aliasStr(dpid.get_alias());             // of the form "ZDC_ZNA_HV0.actual.vMon"
        const auto offs = std::strlen("ZDC_");
        std::string detStr = aliasStr.substr(offs, 3);
        int detID = 0; // order of the detectors: ZNA, ZPA, ZNC, ZPC, ZEM (DIFFEREMT FROM Runs1/2)
        if ((std::strstr(detStr.c_str(), "ZNA")) != nullptr) {
          detID = 1;
        } else if ((std::strstr(detStr.c_str(), "ZPA")) != nullptr) {
          detID = 2;
        } else if ((std::strstr(detStr.c_str(), "ZNC")) != nullptr) {
          detID = 3;
        } else if ((std::strstr(detStr.c_str(), "ZPC")) != nullptr) {
          detID = 4;
        } else if ((std::strstr(detStr.c_str(), "ZEM")) != nullptr) {
          detID = 5;
        }
        std::size_t pos = aliasStr.find("HV");
        std::string chStr = aliasStr.substr(1, pos + 2); // between 0 and 4
        auto ich = std::stoi(chStr);
        auto hvch = 5 * (detID - 1) + ich; // ZNA[0...4],ZPA[0...4],ZNC[0...4],ZPC[0...4],ZEM[1,2]
        std::bitset<NHVCHANNELS> hvstatus(o2::dcs::getValue<int32_t>(dpcom));
        if (mVerbose) {
          LOG(INFO) << "HV ch. " << hvch << " Prev. value = " << mPrevHVstatus << ", New value = " << hvstatus;
        }
        if (hvstatus == mPrevHVstatus) {
          if (mVerbose) {
            LOG(INFO) << "Same HV status as before, doing nothing";
          }
          return 0;
        }
        if (mVerbose) {
          LOG(INFO) << "Something changed in HV for ch. " << hvch;
        }
        mUpdateHVStatus = true;
        for (auto ich = 0; ich < NHVCHANNELS; ++ich) {
          auto singlestripHV = hvstatus[ich];
          if (mHV[ich] != singlestripHV) {
            mHV[ich] = singlestripHV;
          }
        } // end loop on channels
        if (mVerbose) {
          LOG(INFO) << "Updating previous HV status for ch. " << hvch;
        }
        mPrevHVstatus = hvstatus;
      } // end processing current DP, when it is of type HVSTATUS
    }

    if (type == RAW_INT) { // mapping
      // DP  processing
      if (std::strstr(dpid.get_alias(), "CONFIG") != nullptr) { // DP from CONFIG (ie mapping)
        std::string aliasStr(dpid.get_alias());
        const auto offs = std::strlen("ZDC_CONFIG_");
        std::size_t const nsta = aliasStr.find_first_of("0123456789", offs);
        std::size_t const nend = aliasStr.find_first_not_of("0123456789", nsta);
        std::string ddlStr = aliasStr.substr(nsta, nend != std::string::npos ? nend - nsta : nend);
        auto iddl = std::stoi(ddlStr);
        const auto offs1 = nend;
        std::size_t const nsta1 = aliasStr.find_first_of("0123456789", offs1);
        std::size_t const nend1 = aliasStr.find_first_not_of("0123456789", nsta1);
        std::string chStr = aliasStr.substr(nsta1, nend1 != std::string::npos ? nend1 - nsta1 : nend1);
        auto idch = std::stoi(chStr);
        std::bitset<4> mapstatus(o2::dcs::getValue<int32_t>(dpcom));
        if (mVerbose) {
          LOG(INFO) << "DDL: " << iddl << ": Prev = " << mPreviousMapping[iddl] << ", new = " << mapstatus;
        }
        if (mapstatus == mPreviousMapping[iddl]) {
          if (mVerbose) {
            LOG(INFO) << "Same mapping status as before, doing nothing";
          }
          return 0;
        }
        if (mVerbose) {
          LOG(INFO) << "Mapping modified for DDL" << iddl;
        }
        mUpdateMapping = true;
        for (auto imod = 0; imod < NMODULES; ++imod) { // one bit per module
          auto singlechstatus = mapstatus[imod];
          // check on channel mapping continuity...
          for (int ich = 0; ich < NCHANNELS; ++ich) {
            if (idch != (4 * imod + ich)) {
              printf("ZDC -> problem in Nchannels: expecting %d reading %d\n\n", 4 * imod + ich, idch);
            }
            if ((mZDCMapInfo[iddl][imod].moduleID[ich]) == -1) {
              continue;
            }
          }
          if (mVerbose) {
            LOG(INFO) << "mZDCMapInfo[" << iddl << "][" << imod << "].moduleID[" << idch << "] = " << mZDCMapInfo[iddl][imod].channelValue[idch];
          }
          if (mMapping[idch] != singlechstatus) {
            mMapping[idch] = singlechstatus;
          }
        } // end loop on modules
        if (mVerbose) {
          LOG(INFO) << "Updating previous mapping status for DDL " << iddl;
        }
        mPreviousMapping[iddl] = mapstatus;
      } // end processing current DP, when it is of type MAPPING
    }
  }
  return 0;
}

//______________________________________________________________________

uint64_t ZDCDCSProcessor::processFlags(const uint64_t flags, const char* pid)
{

  // function to process the flag. the return code zero means that all is fine.
  // anything else means that there was an issue

  // for now, I don't know how to use the flags, so I do nothing

  if (flags & DataPointValue::KEEP_ALIVE_FLAG) {
    LOG(DEBUG) << "KEEP_ALIVE_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::END_FLAG) {
    LOG(DEBUG) << "END_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::FBI_FLAG) {
    LOG(DEBUG) << "FBI_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::NEW_FLAG) {
    LOG(DEBUG) << "NEW_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::DIRTY_FLAG) {
    LOG(DEBUG) << "DIRTY_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::TURN_FLAG) {
    LOG(DEBUG) << "TURN_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::WRITE_FLAG) {
    LOG(DEBUG) << "WRITE_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::READ_FLAG) {
    LOG(DEBUG) << "READ_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::OVERWRITE_FLAG) {
    LOG(DEBUG) << "OVERWRITE_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::VICTIM_FLAG) {
    LOG(DEBUG) << "VICTIM_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::DIM_ERROR_FLAG) {
    LOG(DEBUG) << "DIM_ERROR_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::BAD_DPID_FLAG) {
    LOG(DEBUG) << "BAD_DPID_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::BAD_FLAGS_FLAG) {
    LOG(DEBUG) << "BAD_FLAGS_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::BAD_TIMESTAMP_FLAG) {
    LOG(DEBUG) << "BAD_TIMESTAMP_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::BAD_PAYLOAD_FLAG) {
    LOG(DEBUG) << "BAD_PAYLOAD_FLAG active for DP " << pid;
  }
  if (flags & DataPointValue::BAD_FBI_FLAG) {
    LOG(DEBUG) << "BAD_FBI_FLAG active for DP " << pid;
  }

  return 0;
}

//______________________________________________________________________

void ZDCDCSProcessor::updateDPsCCDB()
{
  // here we create the object to then be sent to CCDB
  LOG(INFO) << "Updating DCS map";

  std::map<std::string, std::string> md;
  md["responsible"] = "Chiara Oppedisano";
  prepareCCDBobjectInfo(mZDCDCS, mccdbDPsInfo, "ZDC/Calib/DCSDPs", mTF, md); // TBF: Do we save the entire DCS map?!??

  return;
}

//______________________________________________________________________

void ZDCDCSProcessor::updateMappingCCDB()
{

  // we need to update a CCDB for the FEAC status --> let's prepare the CCDBInfo

  if (mVerbose) {
    LOG(INFO) << "Mapping changed --> I will update CCDB";
  }
  std::map<std::string, std::string> md;
  md["responsible"] = "Chiara Oppedisano";
  prepareCCDBobjectInfo(mMapping, mccdbMappingInfo, "ZDC/Calib/Mapping", mTF, md);
  return;
}

//______________________________________________________________________

void ZDCDCSProcessor::updateHVCCDB()
{

  // we need to update a CCDB for the HV status --> let's prepare the CCDBInfo

  if (mVerbose) {
    LOG(INFO) << "At least one HV changed status --> I will update CCDB";
  }
  std::map<std::string, std::string> md;
  md["responsible"] = "Chiara Oppedisano";
  prepareCCDBobjectInfo(mHV, mccdbHVInfo, "ZDC/Calib/HVSetting", mTF, md);
  return;
}

//______________________________________________________________________

void ZDCDCSProcessor::updatePositionCCDB()
{

  // we need to update a CCDB for the table position --> let's prepare the CCDBInfo

  if (mVerbose) {
    LOG(INFO) << "ZDC vertical positions changed --> I will update CCDB";
  }
  std::map<std::string, std::string> md;
  md["responsible"] = "Chiara Oppedisano";
  prepareCCDBobjectInfo(mVerticalPosition, mccdbPositionInfo, "ZDC/Calib/Align", mTF, md);
  return;
}

//_______________________________________________________________________

void ZDCDCSProcessor::getZDCActiveChannels(int nDDL, int nModule, ZDCModuleMap& map) const
{
  //
  // based on ZDC supposed mapping: //TBF
  //

  int nActiveChannels = 0;
  for (int ii = 0; ii < NCHANNELS; ++ii) {
    if ((map.readChannel[ii]) == true) {
      nActiveChannels++;
    }
  }

  if (mVerbose) {
    LOG(INFO) << "nDDL: " << nDDL << " -> Module " << nModule << " has " << nActiveChannels << " active channels";
  }
}
