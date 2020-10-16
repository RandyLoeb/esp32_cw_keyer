var myViewModel = {
  keyerText: ko.observable("hello world"),
  wpm: ko.observable(0),
  wpm_farnsworth: ko.observable(0),
  wpm_farnsworth_slow: ko.observable(0),
  ip: ko.observable(""),
  mac: ko.observable(""),
  tx: ko.observable(1),
  ws_connect: ko.observable(0),
  ws_ip: ko.observable(""),
  tone_hz: ko.observable(0),
  pollDisplay: ko.observable(false),
  displayCache: ko.observable(""),
  polling:0,
  intervalHandle:0,
  handlePolling: function(activate) {
    if (!activate) {
      if (this.polling) {
        clearInterval(this.intervalHandle);
      }
      this.polling =0;
    } else {
      this.polling=1;
      this.intervalHandle = setInterval(this.getDisplayCache, 2000);
    }
  },
  onWpm: function () {
    this.setConfig([{ name: "wpm", value: this.wpm() }]);
  },
  onWpmFarnsworth: function () {
    this.setConfig([{ name: "wpm_farnsworth", value: this.wpm_farnsworth() }]);
  },
  onWpmFarnsworthSlow: function () {
    this.setConfig([
      { name: "wpm_farnsworth_slow", value: this.wpm_farnsworth_slow() },
    ]);
  },
  onTx: function () {
    this.setConfig([{ name: "tx", value: this.tx() }]);
  },
  onWsConnect: function () {
    this.setConfig([{ name: "ws_connect", value: this.ws_connect() }]);
  },
  onWsIp: function () {
    this.setConfig([{ name: "ws_ip", value: this.ws_ip() }]);
  },
  onToneHz: function () {
    this.setConfig([{ name: "tone_hz", value: this.tone_hz() }]);
  },
  onSendText: function () {
    $.ajax({
      url: "/textsubmit",
      type: "post",
      dataType: "json",
      contentType: "application/json",
      success: function (data) {
        // $('#target').html(data.msg);
        this.keyerText("");
      },
      data: JSON.stringify({ text: this.keyerText() }),
    });
  },
  getConfig: function () {
    $.ajax({
      url: "/getconfig",
      type: "get",
      dataType: "json",
      contentType: "application/json",
      success: function (data) {
        myViewModel.updateFresh(data);
      },
    });
  },
  setConfig: function (settings) {
    $.ajax({
      url: "/setconfig",
      type: "post",
      dataType: "json",
      contentType: "application/json",
      success: function (data) {
        myViewModel.updateFresh(data);
      },
      data: JSON.stringify({ settings: settings }),
    });
  },
  getDisplayCache: function () {
    $.ajax({
      url: "/getdisplaycache",
      type: "get",
      dataType: "json",
      contentType: "application/json",
      success: function (data) {
        myViewModel.displayCache(myViewModel.displayCache() + data.cache);
      },
    });
  },
  updateFresh: function (data) {
    console.log(data);
    myViewModel.wpm(data.wpm);
    myViewModel.wpm_farnsworth(data.wpm_farnsworth);
    myViewModel.wpm_farnsworth_slow(data.wpm_farnsworth_slow);
    myViewModel.ip(data.ip);
    myViewModel.mac(data.mac);
    myViewModel.tx(data.tx);
    myViewModel.ws_connect(data.ws_connect);
    myViewModel.ws_ip(data.ws_ip);
    myViewModel.tone_hz(data.tone_hz);
  },
};
