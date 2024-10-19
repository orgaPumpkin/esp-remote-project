#include "dataMessage.h"



bool field_sorter(fieldValue const& lhs, fieldValue const& rhs) {
    return count(lhs.effected.begin(), lhs.effected.end(), true) > count(rhs.effected.begin(), rhs.effected.end(), true);
}


vector<bool> findEffected(unsigned int fieldI, Mem* mem) {
    vector<int> baseOption = mem->fields[fieldI][0];
    vector<bool> result = vector<bool>(baseOption.size(), false);

    for (vector<int> option : mem->fields[fieldI]) {
        for (unsigned int i = 0; i < baseOption.size(); i++) {
            if (result[i] == false && option[i] != baseOption[i]) {
                result[i] = true;
            }
        }
    }
    return result;
}


vector<fieldValue> getFieldsServer(ESP8266WebServer& server, Mem* mem) {
    vector<fieldValue> fields = vector<fieldValue>();

    // for arg in request
    for (int argI = 0; argI < server.args(); argI++) {
        String argName = server.argName(argI);

        // extract all fields data
        int fieldI = findField(argName, mem);
        if (fieldI != -1) { // field exists
            Serial.println("found field");
            String option = server.arg(argName);
            unsigned int optionI = findElement(option, mem->field_names[fieldI])-1;
            if (optionI < mem->fields[fieldI].size()) { // option exists
                Serial.println("found option");
                fields.emplace_back();
                fields.back().fieldI = fieldI;
                fields.back().optionI = optionI;
                fields.back().effected = findEffected(fieldI, mem);
                mem->last_options[fieldI] = optionI;
            }
        }
    }

    return fields;
}


vector<int> buildDataMessage(vector<fieldValue> fields, Mem* mem) {

    // sort the fields from biggest to smallest
    sort(fields.begin(), fields.end(), field_sorter);

    // build message
    vector<int> message = mem->base_message;
    vector<String> disabled = vector<String>();
    for (fieldValue field : fields) {
        // check disabled
        for (String disabled_field : mem->rules[field.fieldI][field.optionI]) {
            disabled.push_back(disabled_field);
        }
        Serial.println(mem->field_names[field.fieldI][0]);
        if (count(disabled.begin(), disabled.end(), mem->field_names[field.fieldI][0]) == 0) {  // if not disabled
            for (unsigned int i = 0; i < field.effected.size(); i++) {
                if (field.effected[i]) {
                    message[i] = mem->fields[field.fieldI][field.optionI][i];
                }
            }
        }
    }
    return message;
}
