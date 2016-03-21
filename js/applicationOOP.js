// FleetLink app
//Randy King 1/4/2016


// Variable setup and default values

    var baseURL = "https://agent.electricimp.com";


    var mapFleetLink;
    var myLatLng = {lat: 38.492, lng: -122.718};
    var routePath;
    var routeCoordinates = [];
    var targetCoordinates;
    var targetColor = '#FF9900';
    var routeColor = '#FF9900'; //'rgb(0,128,255)'
    var targetUnitKey;
    var accessUnitKey;

    var fleetLink = {
              1: {
                'serialNumber': 'A',  // only use last 8 digits of MAC to save packet characters
                'macAddress': '6909795a',
                'agentURL': '/Bs6cc2GgCE6w',
                'onlineStatus': false,
                'position' : {lat: 38.490,lon: -122.7226}
                  },
              2: {
                'serialNumber': 'B',
                'macAddress': '6908cd7e',
                'agentURL': '/LdRa4cy-o9XA',
                'onlineStatus': true,
                'position' : {lat: 38.4898, lon: -122.7181}
                  },
              3: {
                'serialNumber': 'C',
                'macAddress': '690a2732',
                'agentURL': '/Bti2MjZSPH-V',
                'onlineStatus': true,
                'position' : {lat: 38.491, lon: -122.7135}
              } ,
              4: {
                'serialNumber': 'D',
                'macAddress': '6904f367',
                'agentURL': '/7tb6u_BFviM6',
                'onlineStatus': true,
                'position' : {lat: 38.492, lon: -122.717}
              }


        };

        var interestPacket = {
              'type': "i",
              'name' : [null,null,null,null,null,null,null,null,null,null,],
              'nonce' : 987,
              'trace' : "w"
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

          break;

          case "m":
              interestPacket.name[0] =  "m";  //  a native modbus command
              interestPacket.name[1] = fleetLink[targetUnitKey].serialNumber  ;
              interestPacket.name[2] = document.getElementById("modbusID").value ;
              interestPacket.name[3] = document.getElementById("modbusFC").value ;
              interestPacket.name[4] = document.getElementById("modbusRegister").value ;
              interestPacket.name[5] = document.getElementById("modbusSize").value  ;
              interestPacket.nonce = Math.floor((Math.random() * 999) + 1).toString(); // all the components must be strings
          break;

          case "f":

          break;
          default:
              console.log("data model not recognized")
        }
        document.getElementById("completedInterestPacket").textContent = interestPacket.type +
              interestPacket.name ;
      }


      function clearDisplay() {
          routePath.setMap(null);
          targetCircle.setMap(null);
          document.getElementById("returnedDataPacket").textContent = "Waiting for response...."; // indicate action
          document.getElementById("hexadecimalDataValue").textContent = "--";
          document.getElementById("decimalDataValue").textContent = "--";
          document.getElementById("stringDataRepresentation").textContent = "--";
          document.getElementById("trace").textContent = "--";
      }


      function expressInterestPacket() {
        // disable the Go button
        document.getElementById("expressInterestPacket").disabled = "disabled";
        clearDisplay();
        // play a swoosh
        new Audio("img/swoosh.mp3").play();

        buildInterestPacket();
        //console.log("expressing Interest Packet to " + fleetLink[accessUnitKey].serialNumber + "-> " );
        $.ajax({
        // send the interest packet to the selected agent and expect a data packet in response
           url: baseURL + fleetLink[accessUnitKey].agentURL + '/expressInterest',
           timeout: 20000,
           data: JSON.stringify(interestPacket), // convert interest packet string to JSON
           type: 'POST',
           success : function(response) {
               //document.getElementById("expressInterestPacket").disabled = false; // re-enable the Go button
                 if ('returnData' in response) {
                   var dataTable = JSON.parse(response.returnData);
                    //console.log(dataTable);
                    displayData(dataTable);

                    traceRoute(dataTable.trace); //display the route trace
                    new Audio("img/smallBell2.wav").play();  // sound chime to indicate successful data packet reception
                    setTimeout(function(){document.getElementById("expressInterestPacket").disabled = false;}, 7000 );
                  }
            },
           error : function(jqXHR, textStatus, err) {

               var errorResponse = jqXHR.status + ' ' + textStatus + ': ' + err + ' - ' + jqXHR.responseText;
               document.getElementById("returnedDataPacket").textContent = errorResponse;
               setTimeout(function(){document.getElementById("expressInterestPacket").disabled = false;}, 7000 );
           }
         });
       }




        function displayData(dataTable) {
          document.getElementById("returnedDataPacket").textContent = dataTable.contents;
          // display in hex
          var returnedModbusDataHex = "0x" + dataTable.contents;
          document.getElementById("hexadecimalDataValue").textContent = returnedModbusDataHex;
          // and in decimal
          document.getElementById("decimalDataValue").textContent =  parseInt(returnedModbusDataHex) ;
          // and in string form, assuming modbus data is ascii characters in hex form
          var stringRepresentation = ""
          for (i = 0; i < dataTable.contents.length; i=i+2) {
                stringRepresentation += String.fromCharCode(parseInt("0x" + dataTable.contents.slice(i,i+2)))
              }
          document.getElementById("stringDataRepresentation").textContent = stringRepresentation;
          // display trace dataTable
          document.getElementById("trace").textContent = dataTable.trace;
          }



        function drawMarker(fleetLinkKey) {
          var markerLat = fleetLink[fleetLinkKey].position.lat;
          var markerLng = fleetLink[fleetLinkKey].position.lon;
          //console.log("At DrawMarker " + markerLat + "  " + markerLng + " online? " + fleetLink[fleetLinkKey].onlineStatus)
          var markerLatLng = {lat: markerLat, lng: markerLng};
          var iconImage;

          // choose marker based on online status and whether access point
          if (fleetLink[fleetLinkKey].onlineStatus) {
              if(fleetLinkKey == accessUnitKey) {
                iconImage = "img/iconOnlineAccess.png";
              } else {
                iconImage = "img/iconOnlineHouse.png";
              }
          } else {
                iconImage = "img/iconOfflineHouse.png";
          }

        //    console.log(" Access= " + accessUnitKey+ " Target= " + targetUnitKey +  " This Unit " + fleetLinkKey + " Online? " + fleetLink[fleetLinkKey].onlineStatus + " Icon= " + iconImage);
          // draw the chosen marker
          var marker = new google.maps.Marker({
            position: markerLatLng,
            map: mapFleetLink,
            icon: iconImage,
            label: {text: fleetLink[fleetLinkKey].serialNumber, color: '#3333CC'}
          });
        }


      function updateMarkers() {
           //console.log("updating map..");
           clearDisplay();
           buildInterestPacket();
            for (var key in fleetLink) {
                drawMarker(key);
            }
      }

      function traceRoute(trace)   {
            //console.log(trace)
          //  routePath.setMap(null);
            routeCoordinates = [null];
            var lineSymbol = {
                path: google.maps.SymbolPath.FORWARD_CLOSED_ARROW
            };

            for (var i=0; i< trace.length; i++) {
              var thisSerialNumber = trace.slice(i,i+1);
              for (var key in fleetLink) {
                  if (fleetLink[key].serialNumber == thisSerialNumber) {
                      routeCoordinates[i] = new google.maps.LatLng({lat: fleetLink[key].position.lat, lng: fleetLink[key].position.lon});
                  }
              }
            }

            routePath = new google.maps.Polyline({
            path: routeCoordinates,
            icons: [{
                icon: lineSymbol,
                offset: '100%'
            }],
            geodesic: true,
            strokeColor: routeColor,
            strokeOpacity: 1.0,
            strokeWeight: 3
            });

            routePath.setMap(mapFleetLink);

            targetCoordinates = new google.maps.LatLng({lat: fleetLink[targetUnitKey].position.lat, lng: fleetLink[targetUnitKey].position.lon});

            targetCircle = new google.maps.Circle({
              strokeColor: targetColor,
              strokeOpacity: 1.0,
              strokeWeight: 3,
              fillColor: "white",
              fillOpacity: 0.0,
              map: mapFleetLink,
              center: targetCoordinates,
              radius: 30
            });

      }



      function initMap() {
        // Draw default map
          mapFleetLink = new google.maps.Map(document.getElementById('map'), {
            zoom: 16,
           mapTypeId: google.maps.MapTypeId.SATELLITE,
            center: myLatLng
          });

          routePath = new google.maps.Polyline({
          path: routeCoordinates,
          geodesic: true,
          strokeColor: routeColor,
          strokeOpacity: 0.7,
          strokeWeight: 4
          });

          routePath.setMap(mapFleetLink);


            targetCircle = new google.maps.Circle({
            strokeColor: targetColor,
            strokeOpacity: 0.8,
            strokeWeight: 2,
            fillColor: "white",
            fillOpacity: 0.15,
            map: mapFleetLink,
            center: targetCoordinates,
            radius: 20
          });
      }




        function showHideControls(){
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

        $( document ).ready(function() {
            // Do the following after the page is ready
            //console.log( "Initializing..." );
            initMap();
            document.getElementById("dataModel").value = "m" ; // default to Modbus data model
            showHideControls();
            buildInterestPacket();
            updateMarkers();
            });
