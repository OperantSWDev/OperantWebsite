// FleetLink app
//Randy King 1/4/2016


// Variable setup and default values

    var baseURL = "https://agent.electricimp.com";
    var myLatLng = {lat: 38.49, lng: -122.725};
    var mapFleetLink;

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
                  }  /*,
              4: {
                      'serialNumber': 'Unit4',
                      'macAddress': '0xxxxxxxxx',
                      'agentURL': '/yyyyyyyyyy',
                      'onlineStatus': false,
                      'position' : {
                          'latitude': '38.488',
                          'longitude': '-122.724'
                        }
                  },
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
              'name' : "",
              'nonce' : "",
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
                  accessUnitKey = key;// This is a glabal variable I use throughout: the site we're sending the interest packet to first
                }
          });
          // need to combine convenience "base" and "sunspec" registers to get finalmodbus register
          var modbusRegister = Number(document.getElementById("baseRegister").value)+Number(document.getElementById("sunspecRegister").value);
          // now build interest packet
          interestPacket.name = document.getElementById("dataModel").value + "/" ;

          // Now, remainder depends on which data maodel was selected

          switch (document.getElementById("dataModel").value) {
            case "m":
                interestPacket.name += fleetLink[targetUnitKey].macAddress   + "/" +
                document.getElementById("modbusID").value + "/"  +
                document.getElementById("modbusFC").value + "/"  +
                modbusRegister + "/"  +
                document.getElementById("modbusSize").value + "/"  ;

                // hide the wrong form of results display


                switch(modbusRegister) {
                    case 50115:
                    document.getElementById("stringDataRepresentation").style.visibility="hidden";
                    document.getElementById("decimalDataValue").style.visibility="visible";
                    break;
                    case 50004:
                    document.getElementById("stringDataRepresentation").style.visibility="visible";
                    document.getElementById("decimalDataValue").style.visibility="hidden";
                    break;
                    default:
                    document.getElementById("stringDataRepresentation").style.visibility="visible";
                    document.getElementById("decimalDataValue").style.visibility="visible";
                  }

            break;

            case "f":
                interestPacket.name += "r";
            break;
            default:
                console.log("data model not recognized")
          }




          document.getElementById("completedInterestPacket").textContent = interestPacket.type +
                interestPacket.name ;

          // The targeted sites may have changed, so update the map
          updateMap();
          }


        function sendInterestPacket() {
          // disable the Go button
          document.getElementById("sendInterestPacket").disabled = "disabled";
          // play a swoosh
          new Audio("img/swoosh.mp3").play();

          document.getElementById("returnedDataPacket").textContent = "Waiting for response...."; // indicate action
          document.getElementById("hexadecimalDataValue").textContent = "--";
          document.getElementById("decimalDataValue").textContent = "--";
          document.getElementById("stringDataRepresentation").textContent = "--";
            buildInterestPacket();
            interestPacketToBeSent = interestPacket.type + interestPacket.name ;

          //  console.log("sendingInterestPacket to " + fleetLink[accessUnitKey].serialNumber + "-> " +  interestPacketToBeSent);

                 $.ajax({
                   // send the interest packet to the selected agent and expect a data packet in response
                     url: baseURL + fleetLink[accessUnitKey].agentURL + '/sendInterest',
                     //timeout: 20000,
                     type: 'POST',
                     // convert interest packet string to JSON
                     data: JSON.stringify({ 'interestPacketString' : interestPacketToBeSent }),
                     success : function(response) {
                         document.getElementById("sendInterestPacket").disabled = false;
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
                                    document.getElementById("decimalDataValue").textContent =  parseInt(returnedModbusDataHex)/1000 + "  kWh" ;
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
                         document.getElementById("sendInterestPacket").disabled = false;
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
          var selectedDataModel = document.getElementById("dataModel").value;
          switch(selectedDataModel){
            case "f":
            document.getElementById("modbusPanelContainer").style.visibility="hidden";
            document.getElementById("fleetlinkPanelContainer").style.visibility="visible";
            break;
            case "m":
            document.getElementById("modbusPanelContainer").style.visibility="visible";
            document.getElementById("fleetlinkPanelContainer").style.visibility="hidden";

            break;
          }
        }
