// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

// // we need to add workflow options before including Framework/runDataProcessing
// void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
// {
//   // option allowing to set parameters
// }

// ------------------------------------------------------------------

#include "DCStestWorkflow/DCSRandomDataGeneratorSpec.h"
#include "Framework/runDataProcessing.h"

o2::framework::WorkflowSpec defineDataProcessing(o2::framework::ConfigContext const& configcontext)
{
  std::vector<o2::dcs::test::HintType> dphints;
  // for ZDC
  // (as far as my knowledge goes about the DCS dp right now!)
  dphints.emplace_back(o2::dcs::test::DataPointHint<double>{"zdc_pos[00..04]", 0, 4});
  dphints.emplace_back(o2::dcs::test::DataPointHint<int32_t>{"zdc_hv[00..22]", 0, 22});
  dphints.emplace_back(o2::dcs::test::DataPointHint<int32_t>{"zdc_hva[00..22]", 0, 12});
  dphints.emplace_back(o2::dcs::test::DataPointHint<int32_t>{"ZDC_CONFIG_[00..08]", 0, 4});

  o2::framework::WorkflowSpec specs;
  specs.emplace_back(o2::dcs::test::getDCSRandomDataGeneratorSpec(dphints, "ZDC"));
  return specs;
}
