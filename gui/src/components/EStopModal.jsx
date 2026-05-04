import React, { useEffect, useRef } from "react";
import alarmSound from "../assets/alarm.mp3";

export function EStopModal({ onClose }) {
    const canvasRef = useRef(null);
    const audioRef = useRef(null);

    useEffect(() => {
        if (audioRef.current) {
            audioRef.current.loop = true;
            audioRef.current.play().catch(() => {});
        }
        return () => {
            if (audioRef.current) {
                audioRef.current.pause();
                audioRef.current.currentTime = 0;
            }
        };
    }, []);

    useEffect(() => {
        const canvas = canvasRef.current;
        const ctx = canvas.getContext("2d");
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;

        const pieces = Array.from({ length: 200 }, () => ({
            x: Math.random() * canvas.width,
            y: Math.random() * -canvas.height,
            w: Math.random() * 10 + 5,
            h: Math.random() * 5 + 3,
            color: `hsl(${Math.random() * 360}, 90%, 55%)`,
            speed: Math.random() * 4 + 2,
            angle: Math.random() * 360,
            spin: Math.random() * 4 - 2,
        }));

        let animId;
        function draw() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            pieces.forEach(p => {
                ctx.save();
                ctx.translate(p.x + p.w / 2, p.y + p.h / 2);
                ctx.rotate((p.angle * Math.PI) / 180);
                ctx.fillStyle = p.color;
                ctx.fillRect(-p.w / 2, -p.h / 2, p.w, p.h);
                ctx.restore();
                p.y += p.speed;
                p.angle += p.spin;
                if (p.y > canvas.height) {
                    p.y = -p.h;
                    p.x = Math.random() * canvas.width;
                }
            });
            animId = requestAnimationFrame(draw);
        }
        draw();
        return () => cancelAnimationFrame(animId);
    }, []);

    return (
        <>
            <audio ref={audioRef} src={alarmSound} />

            <div style={{
                position: "fixed", top: 0, left: 0,
                width: "100vw", height: "100vh",
                background: "rgba(0,0,0,0.6)",
                display: "flex", alignItems: "center", justifyContent: "center",
                zIndex: 1000,
            }}>
                <canvas ref={canvasRef} style={{ position: "absolute", top: 0, left: 0, pointerEvents: "none" }} />

                <div style={{
                    position: "relative",
                    background: "#1a0a0a",
                    border: "0.3vw solid #ff3030",
                    borderRadius: "1vw",
                    padding: "3vw 4vw",
                    display: "flex",
                    flexDirection: "column",
                    alignItems: "center",
                    gap: "1.5vw",
                    zIndex: 1001,
                }}>
                    <button onClick={onClose} style={{
                        position: "absolute", top: "0.8vw", right: "0.8vw",
                        background: "transparent", border: "none",
                        color: "white", fontSize: "1.5vw",
                        cursor: "pointer", lineHeight: 1,
                    }}>✕</button>

                    <div style={{ fontSize: "4vw", fontWeight: 900, color: "#ff3030", letterSpacing: "0.2vw" }}>
                        ⛔ E-STOP !!!
                    </div>

                    <div style={{ fontSize: "3vw", color: "#ffaaaa", fontWeight: "bold" }}>
                        Go chase the pod!!!!!
                    </div>
                </div>
            </div>
        </>
    );
}