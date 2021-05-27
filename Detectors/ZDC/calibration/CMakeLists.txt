# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

o2_add_library(ZDCCalibration
	       src/ZDCDCSProcessor.cxx
               PUBLIC_LINK_LIBRARIES O2::DataFormatsZDC O2::ZDCBase
               O2::CCDB
	       O2::DetectorsCalibration
	       O2::DetectorsDCS
	       ROOT::Minuit
	       ms_gsl::ms_gsl)


o2_target_root_dictionary(ZDCCalibration
                          HEADERS include/ZDCCalibration/ZDCDCSProcessor.h)


o2_add_executable(data-generator-workflow
                  COMPONENT_NAME calibration
                  SOURCES testWorkflow/data-generator-workflow.cxx
                  PUBLIC_LINK_LIBRARIES O2::Framework
	          O2::DataFormatsZDC
		  O2::ZDCBase)

o2_add_executable(lhc-clockphase-workflow
                  COMPONENT_NAME calibration
                  SOURCES testWorkflow/lhc-clockphase-workflow.cxx
                  PUBLIC_LINK_LIBRARIES O2::Framework
 		   O2::ZDCCalibration
	           O2::DetectorsCalibration)

o2_add_executable(zdc-channel-calib-workflow
                  COMPONENT_NAME calibration
                  SOURCES testWorkflow/zdc-channel-calib-workflow.cxx
                  PUBLIC_LINK_LIBRARIES O2::Framework
 		  O2::ZDCCalibration
	          O2::DetectorsCalibration)

o2_add_executable(zdc-calib-workflow
                  COMPONENT_NAME calibration
                  SOURCES testWorkflow/zdc-calib-workflow.cxx
                  PUBLIC_LINK_LIBRARIES O2::Framework
 		  O2::ZDCCalibration
	          O2::DetectorsCalibration)


o2_add_executable(zdc-dummy-ccdb-for-calib
                  COMPONENT_NAME calibration
                  SOURCES testWorkflow/zdc-dummy-ccdb-for-calib.cxx
                  PUBLIC_LINK_LIBRARIES O2::Framework
 		                              O2::ZDCCalibration
	                                      O2::DetectorsCalibration)

o2_add_executable(zdc-collect-calib-workflow
                  COMPONENT_NAME calibration
                  SOURCES testWorkflow/zdc-collect-calib-workflow.cxx
                  PUBLIC_LINK_LIBRARIES O2::Framework
 		                              O2::ZDCCalibration
	                                      O2::DetectorsCalibration)

o2_add_executable(zdc-dcs-workflow
                  COMPONENT_NAME calibration
                  SOURCES testWorkflow/zdc-dcs-data-workflow.cxx
                  PUBLIC_LINK_LIBRARIES O2::Framework
                                        O2::ZDCCalibration
                                        O2::DetectorsDCS)

o2_add_executable(zdc-dcs-sim-workflow
                  COMPONENT_NAME calibration
                  SOURCES testWorkflow/zdc-dcs-sim-workflow.cxx
                  PUBLIC_LINK_LIBRARIES O2::DCStestWorkflow)
