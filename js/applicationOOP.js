// FleetLink app
//Randy King 1/4/2016


// Variable setup and default values

    var baseURL = "https://agent.electricimp.com";
    var myLatLng = {lat: 38.49, lng: -122.725};
    var mapFleetLink;
    var targetUnitKey;
    var accessUnitKey;

    var fleetLink = {
              /*1: {
                'serialNumber': 'Unit1',  // only use last 8 digits of MAC to save packet characters
                'macAddress': '2a690a2c16',
                'agentURL': '/mBJPRzig9S8d',
                'onlineStatus': false,
                'position' : {
                    'latitude': '38.490',
                    'longitude': '-122.730'
                        }
                  }, */
              2: {
                'serialNumber': 'Unit2',
                'macAddress': '6909795a',
                'agentURL': '/Bs6cc2GgCE6w',
                'onlineStatus': false,
                'position' : {
                    'latitude': '38.492',
                    'longitude': '-122.728'
                        }
                  }  ,
              3: {
                      'serialNumber': 'Unit3',
                      'macAddress': '6908cd7e',
                      'agentURL': '/LdRa4cy-o9XA',
                      'onlineStatus': false,
                      'position' : {
                          'latitude': '38.490',
                          'longitude': '-122.726'
                        }
                  }  ,
              4: {
                      'serialNumber': 'Unit4',
                      'macAddress': '0xxxxxxxxx',
                      'agentURL': '/yyyyyyyyyy',
                      'onlineStatus': false,
                      'position' : {
                          'latitude': '38.488',
                          'longitude': '-122.724'
                        }
                  } /*,
              5: {
                      'serialNumber': 'Unit5',
                      'macAddress': '0xxxxxxxxx',
                      'agentURL': '/yyyyyyyyyy',
                      'onlineStatus': false,
                      'position' : {
                          'latitude': '38.486',
                          'longitude': '-122.722'
                        }
                  } */
        };

        var interestPacket = {
              'type': "i/",
              'name' : [null,null,null,null,null,null,null,null,null,null,],
              'nonce' : "54"
              }

        function buildInterestPacket() {
          // Unit is specified by Serial Number in browser, but we need the MAC address for the interest packet
          targetUnitSerialNumber = document.getElementById("selectedUnit").value;
          accessUnitSerialNumber = document.getElementById("selectedAccessPoint").value ;
          Object.keys(fleetLink).forEach(function(key,index){
                // key: the name of the object key
                // index: the ordinal position of the key within the object
                if (fleetLink[key].serialNumber == targetUnitSerialNumber) {
                  targetUnitKey = key; // This is a global variable I use throughout: the site we want data from
                }
                if (fleetLink[key].serialNumber == accessUnitSerialNumber) {
                  accessUnitKey = key;// This is a global variable I use throughout: the site we're sending the interest packet to first
                }
          });


          switch (document.getElementById("dataModel").value) {

            case "c":

            break;

            case "s":

                var sunSpecRegisterValue = parseInt(document.getElementById("baseSunSpecRegister").value) +
                 parseInt(document.getElementById("commonBlockLength").value)  +
                 - 2 +                                                            // by convention
                 parseInt(document.getElementById("sunSpecValueRegister").value) ;
                console.log(sunSpecRegisterValue);

                interestPacket.name =  "m/" +  // really a modbus command
                fleetLink[targetUnitKey].macAddress   + "/" +
                document.getElementById("sunSpecUnitId").value + "/"  +
                document.getElementById("sunSpecFC").value + "/"  +
                sunSpecRegisterValue + "/"  +
                document.getElementById("sunSpecSize").value + "/"  ;
            break;

            case "m":
                interestPacket.name[0] =  "m";  //  a native modbus command
                interestPacket.name[1] = fleetLink[targetUnitKey].macAddress  ;
                interestPacket.name[2] = document.getElementById("modbusID").value ;
                interestPacket.name[3] = document.getElementById("modbusFC").value ;
                interestPacket.name[4] = document.getElementById("modbusRegister").value ;
                interestPacket.name[5] = document.getElementById("modbusSize").value  ;
                console.log("created interest packet = " + interestPacket.name);
            break;

            case "f":
                interestPacket.name += "r";
            break;
            default:
                console.log("data model not recognized")
          }

          document.getElementById("completedInterestPacket").textContent = interestPacket.type +
                interestPacket.name ;

          }


        function expressInterestPacket() {
          // disable the Go button
          document.getElementById("expressInterestPacket").disabled = "disabled";
          // play a swoosh
          new Audio("img/swoosh.mp3").play();

          document.getElementById("returnedDataPacket").textContent = "Waiting for response...."; // indicate action
          document.getElementById("hexadecimalDataValue").textContent = "--";
          document.getElementById("decimalDataValue").textContent = "--";
          document.getElementById("stringDataRepresentation").textContent = "--";
            buildInterestPacket();
          //  interestPacketToBeSent = interestPacket.type + interestPacket.name ;

            console.log("expressing Interest Packet to " + fleetLink[accessUnitKey].serialNumber + "-> " );
                 $.ajax({
                   // send the interest packet to the selected agent and expect a data packet in response
                     url: baseURL + fleetLink[accessUnitKey].agentURL + '/expressInterest',
                     //timeout: 20000,
                     data: JSON.stringify(interestPacket),
                     type: 'POST',
                     // convert interest packet string to JSON

                     success : function(response) {
                         document.getElementById("expressInterestPacket").disabled = false;
                         console.log(response);
                           if ('dataPacket' in response) {
                              // display in raw data packet form
                              document.getElementById("returnedDataPacket").textContent = response.dataPacket;
                              // parse our returned data in hexadecimal form
                              var parsedDataString = response.dataPacket.split("/"); //parse it into components
                              // example "/d/m/0c2a690a2c16/100/03/50116/1/0648"
                              // 0-packet type/1-data mode/2-MAC Address/3-Modbus ID/4-Modbus FC/5-Modbus Register/6-NumberModbusRegisters/7-Returned Data
                              // display in hex
                              for(i=0; i<=7; i++){
                                console.log(parsedDataString[i]);
                              }
                              switch(parsedDataString[1]) {  // handler depends on which data model
                                case "m":
                                                              console.log(parsedDataString[7])
                                    var returnedModbusDataHex = "0x" + parsedDataString[7];
                                    document.getElementById("hexadecimalDataValue").textContent = returnedModbusDataHex;
                                    // and in decimal
                                    document.getElementById("decimalDataValue").textContent =  parseInt(returnedModbusDataHex) ;
                                    // and in string form, assuming modbus data is ascii characters in hex form
                                    var stringRepresentation = "";
                                    for (i = 0; i < parsedDataString[7].length; i=i+2) {
                                      stringRepresentation += String.fromCharCode(parseInt("0x" + parsedDataString[7].slice(i,i+2)));
                                    }
                                    //console.log(stringRepresentation);
                                    document.getElementById("stringDataRepresentation").textContent = stringRepresentation;
                                break;
                                case "f":
                                    var calibratedRSSI = (parsedDataString[3]/0.74)-5; // very rough calibration based on a couple of measurements!
                                    var unCalibratedRSSI = parsedDataString[3]; // raw RSSI as reported
                                    document.getElementById("stringDataRepresentation").textContent = "RSSI = " + unCalibratedRSSI + " dBm";
                                break;

                                }

                                // sound chime to indicate successful data packet reception
                                    new Audio("img/smallBell2.wav").play();
                            }
                          },
                     error : function(jqXHR, textStatus, err) {
                         document.getElementById("expressInterestPacket").disabled = false;
                         var errorResponse = jqXHR.status + ' ' + textStatus + ': ' + err + ' - ' + jqXHR.responseText;
                         document.getElementById("returnedDataPacket").textContent = errorResponse;
                     }

                 });

             }




          function drawMarker(fleetLinkKey) {
            var markerLat = parseFloat(fleetLink[fleetLinkKey].position.latitude);
            var markerLng = parseFloat(fleetLink[fleetLinkKey].position.longitude);
      //      console.log("At DrawMarker " + markerLat + "  " + markerLng + " online? " + fleetLink[fleetLinkKey].onlineStatus)
            var markerLatLng = {lat: markerLat, lng: markerLng};
            var iconImage;

            // choose marker based on online status and whether access point
            if (fleetLink[fleetLinkKey].onlineStatus) {
                if(fleetLinkKey != targetUnitKey && fleetLinkKey != accessUnitKey) {
                  iconImage = "img/iconOnlineHouse.png";
                }
                if(fleetLinkKey == targetUnitKey && fleetLinkKey != accessUnitKey) {
                  iconImage = "img/iconOnlineTarget.png";
                }
                if(fleetLinkKey != targetUnitKey && fleetLinkKey == accessUnitKey) {
                  iconImage = "img/iconOnlineAccess.png";
                }
                if(fleetLinkKey == targetUnitKey && fleetLinkKey == accessUnitKey) {
                  iconImage = "img/iconOnlineAccessTarget.png";
                }
            } else {
                if(fleetLinkKey != targetUnitKey && fleetLinkKey != accessUnitKey) {
                  iconImage = "img/iconOfflineHouse.png";
                }
                if(fleetLinkKey == targetUnitKey && fleetLinkKey != accessUnitKey) {
                  iconImage = "img/iconOfflineTarget.png";
                }
                if(fleetLinkKey != targetUnitKey && fleetLinkKey == accessUnitKey) {
                  iconImage = "img/iconOfflineAccess.png";
                }
                if(fleetLinkKey == targetUnitKey && fleetLinkKey == accessUnitKey) {
                  iconImage = "img/iconOfflineAccess.png";
                }
            }

          //    console.log(" Access= " + accessUnitKey+ " Target= " + targetUnitKey +  " This Unit " + fleetLinkKey + " Online? " + fleetLink[fleetLinkKey].onlineStatus + " Icon= " + iconImage);
            // draw the chosen marker
            var marker = new google.maps.Marker({
              position: markerLatLng,
              map: mapFleetLink,
              icon: iconImage
            });
          }


          function getUnitSettings(callback, fleetLinkKey) {
            console.log(baseURL + fleetLink[fleetLinkKey].agentURL);

            // Load the IMP's settings
              $.ajax({
              // find our where the Imp agent thinks the device is, then plot a marker there
                url: baseURL + fleetLink[fleetLinkKey].agentURL + '/settings',
               type: 'GET',
                success : function(response) {
                  // Update device position from response
                  if ('settingsStorage' in response) {
                  // console.log("Latitude " + response.settingsStorage.position.latitude);
                //    console.log("Longitude " + response.settingsStorage.position.longitude);
                //    console.log("Online? " + response.settingsStorage.deviceOnline);
                    fleetLink[fleetLinkKey].position = response.settingsStorage.position;
                    fleetLink[fleetLinkKey].onlineStatus = response.settingsStorage.deviceOnline;
                  }
                  // Execute the callback function, passing the device key
                  callback(fleetLinkKey);
                  },
                error: function() {
                  //  console.log("Device " + fleetLinkKey + " offline");
                    fleetLink[fleetLinkKey].onlineStatus = false;
                    callback(fleetLinkKey);
                  }
              });
            }


      function updateMap() {
          //  console.log("updating map..");
            for (var key in fleetLink) {
              getUnitSettings(drawMarker,key);
            }
            }


      // Do the following after the page is ready
      function initializeMap() {
        // Draw default map
          mapFleetLink =  new google.maps.Map(document.getElementById('map'), {
            zoom: 15,
            center: myLatLng
          });
      }


      // toggle "Range Mode" on/off (which makes the addressed imp send packets every second for connectivity testing)
      function setRangeMode(event) {
        var rangeModeDesiredState = document.getElementById("rangeMode").checked
        console.log("Range Mode "  + rangeModeDesiredState);

       $.ajax({
         // send the interest packet to the selected agent and expect a data packet in response
           url: baseURL + fleetLink[accessUnitKey].agentURL + '/setRangeMode',
           //timeout: 20000,
           type: 'POST',
           // send desired range mode state
           data: JSON.stringify({ 'rangeModeState' : rangeModeDesiredState }),
           success : function(response) {
                        console.log("Range Mode "  + rangeModeDesiredState);
                                  },
           error : function(jqXHR, textStatus, err) {
                        var errorResponse = jqXHR.status + ' ' + textStatus + ': ' + err + ' - ' + jqXHR.responseText;
                        console.log("Problem setting range mode " +  errorResponse  ) ;
                                  }
             });
         }


        function showHideControls(){
          console.log("executing show hide");
          var selectedDataModel = document.getElementById("dataModel").value;
          switch(selectedDataModel){
            case "c": // choose
            document.getElementById("sunSpecPanelContainer").style.visibility="hidden";
            document.getElementById("modbusPanelContainer").style.visibility="hidden";
            document.getElementById("fleetlinkPanelContainer").style.visibility="hidden";
            break;
            case "s": // sunspec
            document.getElementById("sunSpecPanelContainer").style.visibility="visible";
            document.getElementById("modbusPanelContainer").style.visibility="hidden";
            document.getElementById("fleetlinkPanelContainer").style.visibility="hidden";
            break;
            case "m": // modbus
            document.getElementById("sunSpecPanelContainer").style.visibility="hidden";
            document.getElementById("modbusPanelContainer").style.visibility="visible";
            document.getElementById("fleetlinkPanelContainer").style.visibility="hidden";
            break;
            case "f": // fleetlink
            document.getElementById("sunSpecPanelContainer").style.visibility="hidden";
            document.getElementById("modbusPanelContainer").style.visibility="hidden";
            document.getElementById("fleetlinkPanelContainer").style.visibility="visible";
            break;
          }
        }

        function startAutoFillSunSpec(){
          console.log("Autofill started...");
        }

        function mapUnit(){
          console.log("mapping unit...");
          buildInterestPacket();
          drawMarker(targetUnitKey);
        }

        $( document ).ready(function() {
            console.log( "Initializing..." );
            initializeMap();
            document.getElementById("dataModel").value = "m" ; // default to Modbus data model
            showHideControls();

            });
