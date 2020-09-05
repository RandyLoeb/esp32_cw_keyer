var myViewModel = {
  keyerText: ko.observable("hello world"),
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
};
