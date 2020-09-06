var myViewModel = {
  keyerText: ko.observable("hello world"),
  wpm: ko.observable(0),
  wpm_farnsworth: ko.observable(0),
  onWpm: function () {
    this.setConfig([{ name: "wpm", value: this.wpm() }]);
  },
  onWpmFarnsworth: function () {
    this.setConfig([{ name: "wpm_farnsworth", value: this.wpm_farnsworth() }]);
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
        // $('#target').html(data.msg);
        //this.keyerText("");
        console.log(data);
        myViewModel.wpm(data.wpm);
        myViewModel.wpm_farnsworth(data.wpm_farnsworth);
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
        console.log(data);
        myViewModel.wpm(data.wpm);
        myViewModel.wpm_farnsworth(data.wpm_farnsworth);
      },
      data: JSON.stringify({ settings: settings }),
    });
  },
};
