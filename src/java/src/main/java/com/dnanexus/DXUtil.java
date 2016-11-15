// Copyright (C) 2013-2016 DNAnexus, Inc.
//
// This file is part of dx-toolkit (DNAnexus platform client libraries).
//
//   Licensed under the Apache License, Version 2.0 (the "License"); you may
//   not use this file except in compliance with the License. You may obtain a
//   copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
//   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
//   License for the specific language governing permissions and limitations
//   under the License.

package com.dnanexus;

import java.io.File;
import java.io.IOException;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

/**
 * Utility class for handling job input and output.
 */
public class DXUtil {

    /**
     * Working directory of the applet. This is the directory where the job_input.json and
     * job_output.json files are to be found. We record this once at application startup to protect
     * ourselves against possible subsequent changes to the working directory.
     */
    private static final String startupDirectory;

    private static final ObjectMapper mapper = new ObjectMapper();

    static {
        startupDirectory = System.getProperty("user.dir");
    }


    /**
     * Obtains the job input and deserializes it to the specified class.
     *
     * @param valueType class to deserialize job input to
     *
     * @return job input
     */
    public static <T> T getJobInput(Class<T> valueType) {
        String jobInputPath = startupDirectory + "/job_input.json";
        try {
            return DXJSON.safeTreeToValue(mapper.readTree(new File(jobInputPath)), valueType);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Writes the job output.
     *
     * @param object object to be JSON serialized
     */
    public static void writeJobOutput(Object object) {
        String jobOutputPath = startupDirectory + "/job_output.json";
        try {
            mapper.writeValue(new File(jobOutputPath), object);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    // Create a link to an object. This can be used as asn
    public static ObjectNode makeDXLink(DXDataObject dataObject) {
        return DXJSON.getObjectBuilder().put("$dnanexus_link", dataObject.getId()).build();
    }

    /**
     * Create a link to a field in a previous stage. This is
     * used in workflows, to link results between stages.
     *
     * @param stage  previous workflow stage
     * @param input  true for an output field, false otherwise
     * @param fieldName name of the field (output or input)
     *
     * @return JSON representation of a link. Can be used as an input to a workflow stage
     */
    public static ObjectNode makeDXLink(DXStage stage,
                                         boolean input,
                                         String  fieldName) {
        String modifier = null;
        if (input)
            modifier = "inputField";
        else
            modifier = "outputField";
        ObjectNode promise = DXJSON.getObjectBuilder()
            .put("stage", stage.getId())
            .put(modifier, fieldName).build();
        return DXJSON.getObjectBuilder().put("$dnanexus_link", promise).build();
    }


}
