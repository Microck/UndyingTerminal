"use client";

import React, { useCallback, useMemo, useState } from "react";

export interface ShinyTextProps {
  text: string;
  disabled?: boolean;
  speed?: number;
  className?: string;
  color?: string;
  shineColor?: string;
  spread?: number;
  yoyo?: boolean;
  pauseOnHover?: boolean;
  direction?: "left" | "right";
  delay?: number;
}

export const ShinyText: React.FC<ShinyTextProps> = ({
  text,
  disabled = false,
  speed = 2,
  className = "",
  color = "#b5b5b5",
  shineColor = "#ffffff",
  spread = 120,
  yoyo = false,
  pauseOnHover = false,
  direction = "left",
  delay = 0,
}) => {
  const [isPaused, setIsPaused] = useState(false);
  const style = useMemo(() => {
    const duration = Math.max(0.2, speed);
    const delaySeconds = Math.max(0, delay);
    const playState = disabled || isPaused ? "paused" : "running";
    const dir = direction === "left" ? "normal" : "reverse";

    return {
      // Custom properties consumed by globals.css
      ["--shiny-duration" as any]: `${duration}s`,
      ["--shiny-delay" as any]: `${delaySeconds}s`,
      ["--shiny-spread" as any]: `${spread}deg`,
      ["--shiny-color" as any]: color,
      ["--shiny-shine" as any]: shineColor,
      // Direct animation controls
      animationPlayState: playState,
      animationName: "shiny-text-move",
      animationDuration: `${duration}s`,
      animationDelay: `${delaySeconds}s`,
      animationDirection: yoyo ? ("alternate" as const) : dir,
      animationIterationCount: "infinite",
      animationTimingFunction: "linear",
      animationFillMode: "both",
    } as React.CSSProperties;
  }, [color, delay, direction, disabled, isPaused, shineColor, speed, spread, yoyo]);

  const handleMouseEnter = useCallback(() => {
    if (pauseOnHover) setIsPaused(true);
  }, [pauseOnHover]);

  const handleMouseLeave = useCallback(() => {
    if (pauseOnHover) setIsPaused(false);
  }, [pauseOnHover]);

  return (
    <span
      className={`inline-block shiny-text ${className}`}
      style={style}
      onMouseEnter={handleMouseEnter}
      onMouseLeave={handleMouseLeave}
    >
      {text}
    </span>
  );
};
