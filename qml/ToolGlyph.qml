import QtQuick 2.14

Canvas {
    id: glyph
    property string name: "select"
    property color ink: "#117b70"
    onNameChanged: requestPaint()
    onInkChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        ctx.strokeStyle = ink
        ctx.fillStyle = ink
        ctx.lineWidth = 1.6
        ctx.lineCap = "round"
        ctx.lineJoin = "round"
        var w = width
        var h = height
        function line(x1, y1, x2, y2) {
            ctx.beginPath()
            ctx.moveTo(x1, y1)
            ctx.lineTo(x2, y2)
            ctx.stroke()
        }
        if (name === "select") {
            ctx.beginPath()
            ctx.moveTo(w * 0.28, h * 0.16)
            ctx.lineTo(w * 0.28, h * 0.82)
            ctx.lineTo(w * 0.46, h * 0.64)
            ctx.lineTo(w * 0.66, h * 0.84)
            ctx.lineTo(w * 0.76, h * 0.74)
            ctx.lineTo(w * 0.56, h * 0.54)
            ctx.lineTo(w * 0.74, h * 0.42)
            ctx.closePath()
            ctx.fill()
        } else if (name === "pan") {
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.48, w * 0.22, 0, Math.PI * 2)
            ctx.stroke()
            line(w * 0.5, h * 0.1, w * 0.5, h * 0.28)
            line(w * 0.5, h * 0.72, w * 0.5, h * 0.9)
            line(w * 0.1, h * 0.5, w * 0.28, h * 0.5)
            line(w * 0.72, h * 0.5, w * 0.9, h * 0.5)
        } else if (name === "road") {
            line(w * 0.18, h * 0.78, w * 0.82, h * 0.22)
            line(w * 0.18, h * 0.58, w * 0.42, h * 0.34)
            line(w * 0.58, h * 0.66, w * 0.82, h * 0.42)
        } else if (name === "symbol") {
            ctx.strokeRect(w * 0.16, h * 0.32, w * 0.68, h * 0.36)
            ctx.beginPath()
            ctx.arc(w * 0.32, h * 0.7, w * 0.1, 0, Math.PI * 2)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(w * 0.68, h * 0.7, w * 0.1, 0, Math.PI * 2)
            ctx.stroke()
        } else if (name === "trace") {
            ctx.beginPath()
            ctx.moveTo(w * 0.12, h * 0.7)
            ctx.quadraticCurveTo(w * 0.35, h * 0.15, w * 0.5, h * 0.5)
            ctx.quadraticCurveTo(w * 0.65, h * 0.85, w * 0.88, h * 0.3)
            ctx.stroke()
        } else if (name === "debris") {
            var dots = [[0.3, 0.35], [0.55, 0.28], [0.72, 0.48], [0.4, 0.62], [0.62, 0.72]]
            for (var i = 0; i < dots.length; ++i) {
                ctx.beginPath()
                ctx.arc(w * dots[i][0], h * dots[i][1], w * 0.07, 0, Math.PI * 2)
                ctx.fill()
            }
        } else if (name === "dimension") {
            line(w * 0.12, h * 0.72, w * 0.88, h * 0.28)
            line(w * 0.12, h * 0.58, w * 0.12, h * 0.86)
            line(w * 0.88, h * 0.14, w * 0.88, h * 0.42)
        } else if (name === "text") {
            ctx.font = "bold " + Math.floor(h * 0.7) + "px sans-serif"
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"
            ctx.fillText("文", w * 0.5, h * 0.52)
        } else if (name === "crosswalk") {
            for (var s = 0; s < 4; ++s)
                ctx.fillRect(w * (0.16 + s * 0.2), h * 0.22, w * 0.1, h * 0.56)
        } else if (name === "guide") {
            line(w * 0.2, h * 0.5, w * 0.72, h * 0.5)
            ctx.beginPath()
            ctx.moveTo(w * 0.62, h * 0.28)
            ctx.lineTo(w * 0.86, h * 0.5)
            ctx.lineTo(w * 0.62, h * 0.72)
            ctx.closePath()
            ctx.fill()
        } else if (name === "circle") {
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.5, w * 0.28, 0, Math.PI * 2)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.5, w * 0.12, 0, Math.PI * 2)
            ctx.stroke()
        } else if (name === "eraser") {
            ctx.beginPath()
            ctx.moveTo(w * 0.22, h * 0.62)
            ctx.lineTo(w * 0.48, h * 0.22)
            ctx.lineTo(w * 0.78, h * 0.42)
            ctx.lineTo(w * 0.52, h * 0.82)
            ctx.closePath()
            ctx.stroke()
            line(w * 0.35, h * 0.42, w * 0.62, h * 0.62)
        } else if (name === "calibrate") {
            line(w * 0.5, h * 0.12, w * 0.5, h * 0.88)
            line(w * 0.12, h * 0.5, w * 0.88, h * 0.5)
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.5, w * 0.16, 0, Math.PI * 2)
            ctx.stroke()
        }
    }
}
