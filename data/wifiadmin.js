document.targetAccessPoint = "";
document.addEventListener("click", function (e) {
  var target = e.target;
  console.log(target);

  if (target.hasAttribute("data-unseenSSID")) {
    /* Get typed-in network name from the box */
    target.setAttribute(
      "data-apname",
      document.getElementById("SSIDinput").value
    );
  }

  if (target.hasAttribute("data-apname")) {
    document.getElementById("rescanDiv").style.display = "none";
    document.getElementById("unseenNetwork").style.display = "none";
    document.getElementById("wifiscantablediv").style.display = "none";

    var apName = target.getAttribute("data-apname");
    document.targetAccessPoint = apName;
    var forgetButton = document.getElementById("forgetButton");
    forgetButton.setAttribute("data-apname", apName);

    var isOpen = target.getAttribute("data-isOpen") == "true";

    if (target.hasAttribute("data-networkType")) {
      var networkType = target.getAttribute("data-networkType");
      document.getElementById("optionsDiv").style.display = "";

      if (networkType == "type1") {
        /* Type1 - Known and found */
        if (isOpen) {
          /* Cancel, Forget, Join */
          document.getElementById("status").innerHTML =
            'Join open network "' + apName + '"?';
          document.getElementById("passwordBox").style.display = "none";
          document.getElementById("cancelButton").style.display = "";
          document.getElementById("forgetButton").style.display = "";
          document.getElementById("joinButton").style.display = "";
        } else {
          /* Password box */
          /* Cancel, Forget, Join */
          document.getElementById("status").innerHTML =
            'Use saved password for network "' +
            apName +
            '" or enter a new one';
          document.getElementById("passwordBox").style.display = "";
          document.getElementById("cancelButton").style.display = "";
          document.getElementById("forgetButton").style.display = "";
          document.getElementById("joinButton").style.display = "";
        }
      } else if (networkType == "type2") {
        /* Type2 - Known but not found */
        if (isOpen) {
          /* Cancel, Forget, Join */
          document.getElementById("status").innerHTML =
            'Open network "' + apName + '" not detected. Try anyway?';
          document.getElementById("passwordBox").style.display = "none";
          document.getElementById("cancelButton").style.display = "";
          document.getElementById("forgetButton").style.display = "";
          document.getElementById("joinButton").style.display = "";
        } else {
          /* Password box */
          /* Cancel, Forget, Join */
          document.getElementById("status").innerHTML =
            'Network "' +
            apName +
            '" not detected.<br />Try anyway?<br />Use saved password or enter a new one';
          document.getElementById("passwordBox").style.display = "";
          document.getElementById("cancelButton").style.display = "";
          document.getElementById("forgetButton").style.display = "";
          document.getElementById("joinButton").style.display = "";
        }
      } else if (networkType == "type3") {
        /* Type3 = Found but not known */
        if (isOpen) {
          /* Cancel, Join */
          document.getElementById("status").innerHTML =
            'Join open network "' + apName + '"?';
          document.getElementById("passwordBox").style.display = "none";
          document.getElementById("cancelButton").style.display = "";
          document.getElementById("forgetButton").style.display = "none";
          document.getElementById("joinButton").style.display = "";
        } else {
          /* Password box */
          /* Cancel, Join */
          document.getElementById("status").innerHTML =
            'Enter password for network "' + apName + '"';
          document.getElementById("passwordBox").style.display = "";
          document.getElementById("cancelButton").style.display = "";
          document.getElementById("forgetButton").style.display = "none";
          document.getElementById("joinButton").style.display = "";
        }
      } else if (networkType == "type4") {
        /* Type4 = Typed in by user */
        if (apName == "") {
          document.getElementById("status").innerHTML =
            "No network name entered";
          document.getElementById("passwordBox").style.display = "none";
          document.getElementById("cancelButton").style.display = "";
          document.getElementById("forgetButton").style.display = "none";
          document.getElementById("joinButton").style.display = "none";
        } else {
          document.getElementById("status").innerHTML =
            'Enter password for network "' + apName + '"';
          document.getElementById("passwordBox").style.display = "";
          document.getElementById("cancelButton").style.display = "";
          document.getElementById("forgetButton").style.display = "none";
          document.getElementById("joinButton").style.display = "";
        }
      }
    }
  }

  if (target.hasAttribute("data-cancelButton")) {
    document.getElementById("optionsDiv").style.display = "none";
    document.getElementById("confirmForgetDiv").style.display = "none";

    /* Clear anything the user may have typed in, while those boxes are not visible */
    document.getElementById("passwordBox").value = "";
    document.getElementById("SSIDinput").value = "";

    document.getElementById("rescanDiv").style.display = "";
    document.getElementById("unseenNetwork").style.display = "";
    document.getElementById("wifiscantablediv").style.display = "";
    document.getElementById("status").innerHTML =
      "Choose a Wi-Fi network to use";
  }

  if (target.hasAttribute("data-forgetButton")) {
    var apName = target.getAttribute("data-apname");
    var confirmForgetButton = document.getElementById("confirmForgetButton");
    confirmForgetButton.setAttribute("data-apname", apName);
    document.getElementById("optionsDiv").style.display = "none";
    document.getElementById("status").innerHTML =
      'Forget network "' + apName + '"? ';
    document.getElementById("status").style.display = "";
    document.getElementById("confirmForgetDiv").style.display = "";
  }

  if (target.hasAttribute("data-confirmremove")) {
    var confirmed = target.getAttribute("data-confirmremove") == "true";
    if (!confirmed) {
      //alert("about to reload");
      location.reload();
    }
  }

  if (target.hasAttribute("data-cancelpwd")) {
    location.reload();
  }
});

var getWifiScan = function () {
  document.getElementById("rescanDiv").style.display = "none"; // hide SSIDs and buttons immediately
  document.getElementById("unseenNetwork").style.display = "none"; // note that walking up from the bottom of page reduces visible blips in browser
  document.getElementById("wifiscantablediv").style.display = "none";
  document.getElementById("optionsDiv").style.display = "none";
  document.getElementById("status").style.display = "";
  document.getElementById("status").innerHTML = "Scanning for networks...";
  var xmlhttp = new XMLHttpRequest();
  var url = "wifiscan";

  xmlhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      var myArr = JSON.parse(this.responseText); // responseText is the listPlus vector in ../src/wificonfig.h

      var myNetworksFound = [];
      var myNetworksNotFound = [];
      var otherNetworks = [];

      myArr.knownAps.forEach(function (knownName) {
        var found = false;
        myArr.aps.forEach(function (ap) {
          if (ap.name == knownName) {
            myNetworksFound.push(ap);
            found = true;
          }
        });
        if (!found) {
          myNetworksNotFound.push(knownName);
        }
      });

      myArr.aps.forEach(function (apFound) {
        /* I did this just for practice and parallel code, but we get isKnown in the JSON response
        var isKnown = false;
        myNetworksFound.forEach(function (knownap) {
          if (knownap.name == apFound.name) {
            isKnown = true;
          }
        });
        */
        if (!apFound.isKnown) {
          otherNetworks.push(apFound);
        }
      });

      if (myArr.length == 0) {
        document.getElementById("status").innerHTML =
          "No 2.4 GHz Wi-Fi networks found";
      } else {
        document.getElementById("status").innerHTML =
          "Choose a Wi-Fi network for to use";
        var outStr = "";
        outStr += '<div id="wifiscantable" class="divTable SSIDtable">';
        outStr += '<div class="divTableHeading">';
        outStr += '<div class="divTableRow">';
        outStr += '<div class="networksHead">&nbsp;&nbsp;MY NETWORKS</div><hr>'; //sometimes an &nbsp; gets the job done
        outStr += "</div>"; //row
        outStr += "</div>"; //heading

        /* TYPE 1 - KNOWN NETWORKS THAT WERE DETECTED */
        outStr += '<div class="divTableBody">';
        myNetworksFound.forEach(function (element) {
          // create a row for each SSID
          outStr += '<div class="divTableRow">';
          outStr +=
            '<div class="divTableCell">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' +
            element.name +
            "</div>";
          if (!element.isOpen) {
            // closed lock icon
            outStr +=
              '<div class="lockedSSID">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';
          } else {
            // blank filler
            outStr +=
              '<div class="blankCell">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';
          }

          // network radio signal strength
          var rssiInt = parseInt(element.RSSI, 10);
          if (rssiInt < -80) {
            outStr +=
              '<div class="signal1">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>'; // signal strengh icons are in CSS -- too many in one file choke it
          } else if (rssiInt < -49) {
            outStr +=
              '<div class="signal2">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';
          } else {
            outStr +=
              '<div class="signal3">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';
          }

          outStr +=
            '<div class="divTableCell"> <button data-apname="' +
            element.name +
            '" data-networkType=' +
            '"type1"' +
            ' class="myButton">' +
            "Options" +
            "</button></div>";

          outStr += "</div>"; // row
          outStr += "<hr />"; // putting horizontal line here here lays out nicely
        });

        /* TYPE 2 - KNOWN NETWORKS THAT WERE NOT DETECTED */
        myNetworksNotFound.forEach(function (element) {
          // create a row for each SSID
          outStr += '<div class="divTableRow">';
          outStr +=
            '<div class="divTableCell">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' +
            element +
            "</div>";
          // blank spaces for networks that were not scanned
          outStr +=
            '<div class="divTableCell">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';
          outStr +=
            '<div class="notFoundSSID">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';

          outStr +=
            '<div class="divTableCell"> <button data-apname="' +
            element +
            '" data-networkType=' +
            '"type2"' +
            ' class="myButton">' +
            "Options" +
            "</button></div>";

          outStr += "</div>"; //row
          outStr += "<hr />"; // putting horizontal line here here lays out nicely
        });

        outStr += "</div>"; //divTableBody

        outStr += "<br /><br />";
        outStr += '<div class="divTableHeading">';
        outStr += '<div class="divTableRow">';
        outStr +=
          '<div class="networksHead">&nbsp;&nbsp;OTHER NETWORKS</div><hr>'; //sometimes an &nbsp; gets the job done
        outStr += "</div>"; //row
        outStr += "</div>"; //heading
        outStr += '<div class="divTableBody">';

        /* TYPE 3 - NETWORKS DETECTED BUT NOT ALREADY KNOWN */
        otherNetworks.forEach(function (element) {
          // create a row for each SSID
          outStr += '<div class="divTableRow">';
          outStr +=
            '<div class="divTableCell">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' +
            element.name +
            "</div>";
          if (!element.isOpen) {
            // closed lock icon
            outStr +=
              '<div class="lockedSSID">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';
          } else {
            // blank filler
            outStr +=
              '<div class="blankCell">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';
          }

          // network radio signal strength
          var rssiInt = parseInt(element.RSSI, 10);
          if (rssiInt < -80) {
            outStr +=
              '<div class="signal1">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>'; // signal strengh icons are in CSS -- too many in one file choke it
          } else if (rssiInt < -49) {
            outStr +=
              '<div class="signal2">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';
          } else {
            outStr +=
              '<div class="signal3">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div>';
          }

          outStr +=
            '<div class="divTableCell"><button data-apname="' +
            element.name +
            '" data-networkType=' +
            '"type3"' +
            ' class="myButton">' +
            "&nbsp; Join &nbsp;" +
            "</button></div>";

          outStr += "</div>"; //row
          outStr += "<hr />"; // putting horizontal line here here lays out nicely
        });
        outStr += "</div>"; //divTableBody
        outStr += "</div>"; //wifiscantable
        outStr += "<br /><br />";

        document.getElementById("wifiscantablediv").innerHTML = outStr;
        document.getElementById("wifiscantablediv").style.display = "";
      }
      document.getElementById("unseenNetwork").style.display = "";
      document.getElementById("rescanDiv").style.display = "";
    }
  };

  xmlhttp.open("GET", url, true);
  xmlhttp.send();
};

var sendInfo = function (mode) {
  var pass = document.getElementById("passwordBox").value;
  var ap = document.targetAccessPoint; //document.getElementById("apselect").value;
  var other = document.getElementById("SSIDinput").value;

  var xmlhttp = new XMLHttpRequest(); // new HttpRequest instance

  xmlhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      //var myArr = JSON.parse(this.responseText);
      location.reload();
    }
  };

  var theUrl = "";
  if (mode == "save") {
    theUrl = "/updateap";
  }
  if (mode == "forget") {
    theUrl = "/forgetap";
  }
  if (mode == "reboot") {
    theUrl = "/reboot";
  }
  xmlhttp.open("POST", theUrl);
  xmlhttp.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
  var apToUse = other != null && other.length > 0 ? other : ap;
  xmlhttp.send(JSON.stringify({ ap: apToUse, pass: pass }));
};

var confirmForgetButton = document.getElementById("confirmForgetButton");
confirmForgetButton.onclick = function () {
  sendInfo("forget");
};

var rescanButton = document.getElementById("rescanButton");
rescanButton.onclick = function () {
  getWifiScan();
};

var joinButton = document.getElementById("joinButton");
joinButton.onclick = function () {
  document.getElementById("optionsDiv").style = "display:none";
  document.getElementById("status").innerHTML =
    "Rebooting to join network..."; // this often does not get rendered before the screen clears, that's ok
  sendInfo("save");
  sendInfo("reboot");
};

getWifiScan();
