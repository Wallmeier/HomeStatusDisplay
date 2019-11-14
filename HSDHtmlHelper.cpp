#include "HSDHtmlHelper.hpp"

#define SELECTED_STRING (F("selected='selected'"))
#define CHECKED_STRING  (F("checked='checked'")) 
#define EMPTY_STRING    (F(""))

String HSDHtmlHelper::getHeader(const char* title, const String& host, const String& version) const {
    String header;
    header.reserve(1500);

    header  = F("<!doctype html> <html>");
    header += F("<head><meta charset='utf-8'>");
    header += F("<title>");
    header += host;
    header += F("</title>");
    header += F("<style>.button {border-radius:0;height:30px;width:100px;border:0;background-color:black;color:#fff;margin:5px;cursor:pointer;}</style>");
    header += F("<style>.buttonr {border-radius:0;height:30px;width:100px;border:0;background-color:red;color:#fff;margin:5px;cursor:pointer;}</style>");
    header += F("<style>.hsdcolor {width:15px;height:15px;border:1px black solid;float:left;margin-right:5px';}</style>");
    header += F("<style>.rdark {background-color:#f9f9f9;}</style>");
    header += F("<style>.rlight {background-color:#e5e5e5;}</style>");
    header += F("</head>");
    header += F("<body bgcolor='#e5e5e5'><font face='Verdana,Arial,Helvetica'>");
    header += F("<font size='+3'>");
    header += host;
    header += F("</font><font size='-3'>V");
    header += version;
    header += F("</font>");
    header += F("<form><p><input type='button' class='button' onclick=\"location.href='./'\" value='Status'>");
    header += F("<input type='submit' class='button' value='Reboot' name='reset'>");
    header += F("<input type='button' class='button' onclick=\"location.href='./update'\" value='Update Firmware'></p>");
  
    header += F("<p><input type='button' class='button' onclick=\"location.href='./cfgmain'\" value='General'>");
    header += F("<input type='button' class='button' onclick=\"location.href='./cfgcolormapping'\" value='Color mapping'>");
    header += F("<input type='button' class='button' onclick=\"location.href='./cfgdevicemapping'\" value='Device mapping'></p></form>");
  
    header += F("<h4>"); 
    header += title;
    header += F("</h4>");

    Serial.print(F("Header size: "));
    Serial.println(header.length());
  
    return header;  
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getFooter() const {
    return F("</font></body></html>");
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getColorMappingTableHeader() const {
    return F("<table border='1' cellpadding='1' cellspacing='2'>"
             " <tr style='background-color:#828282'>"
             "  <td><b><font size='+1'>Nr</font></b></td>"
             "  <td><b><font size='+1'>Message</font></b></td>"
             "  <td><b><font size='+1'>Color</font></b></td>"
             "  <td><b><font size='+1'>Behavior</font></b></td>"
             " </tr>");
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getColorMappingTableEntry(int entryNum, const HSDConfig::ColorMapping* mapping, 
                                                const String& colorString) const {
    String html;
    html = entryNum % 2 == 0 ? F("<tr class='rlight'><td>") : F("<tr class='rdark'><td>");
    html += entryNum;
    html += F("</td><td>");
    html += mapping->msg;
    html += F("</td><td>");
    html += F("<div class='hsdcolor' style='background-color:");
    html += colorString;
    html += F("';></div> ");
    html += colorString;
    html += F("</td><td>"); 
    html += behavior2String(mapping->behavior);
    html += F("</td></tr>");
  
    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getColorMappingTableFooter() const {
    return F("</table>");
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getColorMappingTableAddEntryForm(int newEntryNum, bool isFull) const {
    String html;  
    html += F("<form><table><tr>");
    html += F("<td><input type='text' name='i' value='");
    html += isFull ? newEntryNum - 1 : newEntryNum;
    html += F("' size='5' maxlength='3' placeholder='Nr'</td>");
    html += F("<td><input type='text' name='n' value='' size='20' maxlength='15' placeholder='message name'></td>");
    html += F("<td><input type='text' name='c' value='' size='10' maxlength='7' placeholder='#ffaabb' list='colorOptions'>");
    html += getColorOptions();
    html += F("</td>");
    html += F("<td><select name='b'>");
    html += getBehaviorOptions(HSDConfig::ON);
    html += F("</select></td></tr></table>");  
    html += F("<input type='submit' class='button' value='");
    html += isFull ? F("Edit") : F("Add/Edit");
    html += F("' name='add'></form>");

    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getDeviceMappingTableHeader() const {
    return F("<table border='1' cellpadding='1' cellspacing='2'>"
             " <tr style='background-color:#828282'>"
             "  <td><b><font size='+1'>Nr</font></b></td>"  
             "  <td><b><font size='+1'>Device</font></b></td>"
             "  <td><b><font size='+1'>Led</font></b></td>"
             " </tr>");
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getDeviceMappingTableEntry(int entryNum, const HSDConfig::DeviceMapping* mapping) const {
    String html = entryNum % 2 == 0 ? F("<tr class='rlight'><td>") : F("<tr class='rdark'><td>");
    html += entryNum;
    html += F("</td><td>");
    html += mapping->name;
    html += F("</td><td>");
    html += mapping->ledNumber;
    html += F("</td></tr>");

    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getDeviceMappingTableFooter() const {
    return F("</table>");
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getDeviceMappingTableAddEntryForm(int newEntryNum, bool isFull) const {
    String html = F("<form><table><tr>");
    html += F("<td><input type='text' name='i' value='");
    html += isFull ? newEntryNum - 1 : newEntryNum;
    html += F("' size='5' maxlength='3' placeholder='Nr'</td>");
    html += F("<td><input type='text' name='n' value='' size='30' maxlength='25' placeholder='device name'></td>");
    html += F("<td><input type='text' name='l' value='");
    html += isFull ? newEntryNum - 1 : newEntryNum;
    html += F("' size='6' maxlength='3' placeholder='led nr'></td></tr></table>");
    html += F("<input type='submit' class='button' value='");
    html += isFull ? F("Edit") : F("Add/Edit");
    html += F("' name='add'></form>");

    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getDeleteForm() const {
    String html;  
    html += F("<form><input type='text' name='i' value='' size='5' maxlength='3' placeholder='Nr'><br/>");
    html += F("<input type='submit' class='button' value='Delete' name='delete'>");
    html += F("<input type='submit' class='button' value='Delete all' name='deleteall'></form>");
  
    return html;  
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getSaveForm() const {
    String html;
    html += F("<form><input type='submit' class='buttonr' value='Save' name='save'>");   
    html += F("<form><input type='submit' class='buttonr' value='Undo' name='undo'></form>");   
    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getColorOptions() const {
    String html = F("<datalist id='colorOptions'>");
    for (uint8_t i = 1; i < NUMBER_OF_DEFAULT_COLORS; i++) {
        String temp = HSDConfig::DefaultColor[i].key;
        temp.toLowerCase();
        html += F("<option value='");
        html += temp;
        html += F("'>");
        html += temp;
        html += F("</option>");
    }
    html += F("</datalist>");  

    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::getBehaviorOptions(HSDConfig::Behavior selectedBehavior) const {
  String onSelect       =   (selectedBehavior == HSDConfig::ON)         ? SELECTED_STRING : EMPTY_STRING;
  String blinkingSelect =   (selectedBehavior == HSDConfig::BLINKING)   ? SELECTED_STRING : EMPTY_STRING;
  String flashingSelect =   (selectedBehavior == HSDConfig::FLASHING)   ? SELECTED_STRING : EMPTY_STRING;
  String flickeringSelect = (selectedBehavior == HSDConfig::FLICKERING) ? SELECTED_STRING : EMPTY_STRING;
  
  String html;
  html += F("<option "); html += onSelect;         html += F(" value='"); html += HSDConfig::ON;         html += F("'>On</option>");
  html += F("<option "); html += blinkingSelect;   html += F(" value='"); html += HSDConfig::BLINKING;   html += F("'>Blink</option>");
  html += F("<option "); html += flashingSelect;   html += F(" value='"); html += HSDConfig::FLASHING;   html += F("'>Flash</option>");
  html += F("<option "); html += flickeringSelect; html += F(" value='"); html += HSDConfig::FLICKERING; html += F("'>Flicker</option>");
  
  return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::ip2String(IPAddress ip) const {
    char buffer[20];
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return String(buffer);
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::behavior2String(HSDConfig::Behavior behavior) const {
    String behaviorString = F("Off");  
    switch(behavior) {
        case HSDConfig::ON:         behaviorString = F("On"); break;
        case HSDConfig::BLINKING:   behaviorString = F("Blink"); break;
        case HSDConfig::FLASHING:   behaviorString = F("Flash"); break;
        case HSDConfig::FLICKERING: behaviorString = F("Flicker"); break;
        default: break;
    }
    return behaviorString;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDHtmlHelper::minutes2Uptime(unsigned long minutes) const {
    char buffer[50];
    memset(buffer, 0, sizeof(buffer));
  
    unsigned long days  = minutes / 60 / 24;
    unsigned long hours = (minutes / 60) % 24;
    unsigned long mins  = minutes % 60;

    sprintf(buffer, "%lu days, %lu hours, %lu minutes", days, hours, mins);
  
    return String(buffer);
}

