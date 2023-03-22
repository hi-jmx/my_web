$(function () {

  $('#goHome').on('click', function () {

    goHome()
  })

})

function goHome (e) {
  console.log(e)
  location.href = 'home.html';
}

function KeyDown () {
  goHome()
}