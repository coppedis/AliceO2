#Copyright CERN and copyright holders of ALICE O2.This software is distributed
#under the terms of the GNU General Public License v3(GPL Version 3), copied
#verbatim in the file "COPYING".
#
#See http: //alice-o2.web.cern.ch/license for full licensing information.
#
#In applying this license CERN does not waive the privileges and immunities
#granted to it by virtue of its status as an Intergovernmental Organization or
#submit itself to any jurisdiction.

o2_add_library(FT0Raw
               SOURCES src/DataBlockFT0.cxx src/DigitBlockFT0.cxx src/RawReaderFT0Base.cxx src/RawWriterFT0.cxx
               PUBLIC_LINK_LIBRARIES O2::CommonDataFormat O2::Headers O2::DataFormatsFT0 O2::FITRaw)
