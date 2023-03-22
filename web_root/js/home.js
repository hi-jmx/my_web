$(function () {

  $('#nav li').on('click', function (e) {

    console.log(this.dataset.index)

    $('#nav li').removeClass('active')
    $(this).addClass('active')

    $('.device').css('display', 'none')

    if (this.dataset.index == 1) {
      $('.device-info').css('display', 'block')
    } else if (this.dataset.index == 2) {
      $('.device-set').css('display', 'block')
    } else if (this.dataset.index == 3) {
      $('.device-update').css('display', 'block')

    } else if (this.dataset.index == 4) {
      $('.device-reboot').css('display', 'block')


    }


  })


  $('#reboot').on('click', function () {

  })


  $('#exampleInputFile').on('input', function (e) {
    // alert('d')

    console.log(this.files)
    // console.log($(e).fi)
  })


  // getDeviceInfo()




})

