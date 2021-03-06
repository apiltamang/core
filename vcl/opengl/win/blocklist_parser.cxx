/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <opengl/win/blocklist_parser.hxx>

#include <sal/log.hxx>

WinBlocklistParser::WinBlocklistParser(const OUString& rURL,
        std::vector<wgl::DriverInfo>& rDriverList)
    : meBlockType(BlockType::UNKNOWN)
    , mrDriverList(rDriverList)
    , maURL(rURL)
{
}

void WinBlocklistParser::parse()
{
    xmlreader::XmlReader aReader(maURL);
    handleContent(aReader);
}

namespace {

wgl::OperatingSystem getOperatingSystem(const OString& rString)
{
    if (rString == "all")
    {
        return wgl::DRIVER_OS_ALL;
    }
    else if (rString == "7")
    {
        return wgl::DRIVER_OS_WINDOWS_7;
    }
    else if (rString == "8")
    {
        return wgl::DRIVER_OS_WINDOWS_8;
    }
    else if (rString == "8_1")
    {
        return wgl::DRIVER_OS_WINDOWS_8_1;
    }
    else if (rString == "10")
    {
        return wgl::DRIVER_OS_WINDOWS_10;
    }

    return wgl::DRIVER_OS_UNKNOWN;
}

wgl::VersionComparisonOp getComparison(const OString& rString)
{
    if (rString == "less")
    {
        return wgl::DRIVER_LESS_THAN;
    }
    else if (rString == "less_equal")
    {
        return wgl::DRIVER_LESS_THAN_OR_EQUAL;
    }
    else if (rString == "greater")
    {
        return wgl::DRIVER_GREATER_THAN;
    }
    else if (rString == "greater_equal")
    {
        return wgl::DRIVER_GREATER_THAN_OR_EQUAL;
    }
    else if (rString == "equal")
    {
        return wgl::DRIVER_EQUAL;
    }
    else if (rString == "not_equal")
    {
        return wgl::DRIVER_NOT_EQUAL;
    }
    else if (rString == "between_exclusive")
    {
        return wgl::DRIVER_BETWEEN_EXCLUSIVE;
    }
    else if (rString == "between_inclusive")
    {
        return wgl::DRIVER_BETWEEN_INCLUSIVE;
    }
    else if (rString == "between_inclusive_start")
    {
        return wgl::DRIVER_BETWEEN_INCLUSIVE_START;
    }

    throw InvalidFileException();
}

OUString getVendor(const OString& rString)
{
    if (rString == "all")
    {
        return "";
    }
    else if (rString == "intel")
    {
        return "0x8086";
    }
    else if (rString == "nvidia")
    {
        return "0x10de";
    }
    else if (rString == "amd")
    {
        return "0x1022";
    }
    else if (rString == "ati")
    {
        return "0x1002";
    }
    else if (rString == "microsoft")
    {
        return "0x1414";
    }
    else
    {
        // Allow having simply the hex number as such there, too. After all, it's hex numbers that
        // are output to opengl_device.log.
        return OStringToOUString(rString, RTL_TEXTENCODING_UTF8);
    }
}

uint64_t getVersion(const OString& rString)
{
    OUString aString = OStringToOUString(rString, RTL_TEXTENCODING_UTF8);
    uint64_t nVersion;
    bool bResult = wgl::ParseDriverVersion(aString, nVersion);

    if (!bResult)
    {
        throw InvalidFileException();
    }

    return nVersion;
}

void handleDevices(wgl::DriverInfo& rDriver, xmlreader::XmlReader& rReader)
{
    int nLevel = 1;
    bool bInMsg = false;

    while(true)
    {
        xmlreader::Span name;
        int nsId;

        xmlreader::XmlReader::Result res = rReader.nextItem(
                xmlreader::XmlReader::Text::Normalized, &name, &nsId);

        if (res == xmlreader::XmlReader::Result::Begin)
        {
            ++nLevel;
            if (nLevel > 2)
                throw InvalidFileException();

            if (name == "msg")
            {
                bInMsg = true;
            }
            else if (name == "device")
            {
                int nsIdDeveice;
                while (rReader.nextAttribute(&nsIdDeveice, &name))
                {
                    if (name == "id")
                    {
                        name = rReader.getAttributeValue(false);
                        OString aDeviceId(name.begin, name.length);
                        rDriver.maDevices.push_back(OStringToOUString(aDeviceId, RTL_TEXTENCODING_UTF8));
                    }
                }
            }
            else
                throw InvalidFileException();
        }
        else if (res == xmlreader::XmlReader::Result::End)
        {
            --nLevel;
            bInMsg = false;
            if (!nLevel)
                break;
        }
        else if (res == xmlreader::XmlReader::Result::Text)
        {
            if (bInMsg)
            {
                OString sMsg(name.begin, name.length);
                rDriver.maMsg = OStringToOUString(sMsg, RTL_TEXTENCODING_UTF8);
            }
        }
    }
}

}

void WinBlocklistParser::handleEntry(wgl::DriverInfo& rDriver, xmlreader::XmlReader& rReader)
{
    if (meBlockType == BlockType::WHITELIST)
    {
        rDriver.mbWhitelisted = true;
    }
    else if (meBlockType == BlockType::BLACKLIST)
    {
        rDriver.mbWhitelisted = false;
    }
    else if (meBlockType == BlockType::UNKNOWN)
    {
        throw InvalidFileException();
    }

    xmlreader::Span name;
    int nsId;

    while (rReader.nextAttribute(&nsId, &name))
    {
        if (name == "os")
        {
            name = rReader.getAttributeValue(false);
            OString sOS(name.begin, name.length);
            rDriver.meOperatingSystem = getOperatingSystem(sOS);
        }
        else if (name == "vendor")
        {
            name = rReader.getAttributeValue(false);
            OString sVendor(name.begin, name.length);
            rDriver.maAdapterVendor = getVendor(sVendor);
        }
        else if (name == "compare")
        {
            name = rReader.getAttributeValue(false);
            OString sCompare(name.begin, name.length);
            rDriver.meComparisonOp = getComparison(sCompare);
        }
        else if (name == "version")
        {
            name = rReader.getAttributeValue(false);
            OString sVersion(name.begin, name.length);
            rDriver.mnDriverVersion = getVersion(sVersion);
        }
        else if (name == "minVersion")
        {
            name = rReader.getAttributeValue(false);
            OString sMinVersion(name.begin, name.length);
            rDriver.mnDriverVersion = getVersion(sMinVersion);
        }
        else if (name == "maxVersion")
        {
            name = rReader.getAttributeValue(false);
            OString sMaxVersion(name.begin, name.length);
            rDriver.mnDriverVersionMax = getVersion(sMaxVersion);
        }
        else
        {
            OString aAttrName(name.begin, name.length);
            SAL_WARN("vcl.opengl", "unsupported attribute: " << aAttrName);
        }
    }

    handleDevices(rDriver, rReader);
}

void WinBlocklistParser::handleList(xmlreader::XmlReader& rReader)
{
    xmlreader::Span name;
    int nsId;

    while (true)
    {
        xmlreader::XmlReader::Result res = rReader.nextItem(
                xmlreader::XmlReader::Text::NONE, &name, &nsId);

        if (res == xmlreader::XmlReader::Result::Begin)
        {
            if (name == "entry")
            {
                wgl::DriverInfo aDriver;
                handleEntry(aDriver, rReader);
                mrDriverList.push_back(aDriver);
            }
            else if (name == "entryRange")
            {
                wgl::DriverInfo aDriver;
                handleEntry(aDriver, rReader);
                mrDriverList.push_back(aDriver);
            }
            else
            {
                throw InvalidFileException();
            }
        }
        else if (res == xmlreader::XmlReader::Result::End)
        {
            break;
        }
    }
}

void WinBlocklistParser::handleContent(xmlreader::XmlReader& rReader)
{
    while (true)
    {
        xmlreader::Span name;
        int nsId;

        xmlreader::XmlReader::Result res = rReader.nextItem(
                xmlreader::XmlReader::Text::NONE, &name, &nsId);

        if (res == xmlreader::XmlReader::Result::Begin)
        {
            if (name == "whitelist")
            {
                meBlockType = BlockType::WHITELIST;
                handleList(rReader);
            }
            else if (name == "blacklist")
            {
                meBlockType = BlockType::BLACKLIST;
                handleList(rReader);
            }
            else if (name == "root")
            {
            }
            else
            {
                throw InvalidFileException();
            }
        }
        else if (res == xmlreader::XmlReader::Result::End)
        {
            if (name == "whitelist" || name == "blacklist")
            {
                meBlockType = BlockType::UNKNOWN;
            }
        }
        else if (res == xmlreader::XmlReader::Result::Done)
        {
            break;
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
