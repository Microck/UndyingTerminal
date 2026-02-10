"use client";

import { useEffect, useState } from "react";
import { motion } from "motion/react";
import type { LucideIcon } from "lucide-react";
import { Link2, Server, Terminal } from "lucide-react";
import { cn } from "@/lib/utils";

export type DiscreteTabsItem = {
  id: string;
  label: string;
  icon: LucideIcon;
};

const DEFAULT_TABS: DiscreteTabsItem[] = [
  { id: "server", label: "Server", icon: Server },
  { id: "session", label: "Session", icon: Terminal },
  { id: "reconnect", label: "Reconnect", icon: Link2 },
];

export default function DiscreteTabs({
  items = DEFAULT_TABS,
  value,
  onValueChange,
  className,
}: {
  items?: DiscreteTabsItem[];
  value?: string;
  onValueChange?: (value: string) => void;
  className?: string;
}) {
  const [internalValue, setInternalValue] = useState(items[0]?.id ?? "");
  const activeValue = value ?? internalValue;

  useEffect(() => {
    if (items.length === 0) return;
    if (!items.some((t) => t.id === activeValue)) {
      const next = items[0].id;
      if (value === undefined) setInternalValue(next);
      onValueChange?.(next);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [items]);

  const setActive = (next: string) => {
    if (value === undefined) setInternalValue(next);
    onValueChange?.(next);
  };

  return (
    <div className={cn("flex items-center gap-3", className)}>
      {items.map((tab) => (
        <TabButton
          key={tab.id}
          tab={tab}
          isActive={activeValue === tab.id}
          onClick={() => setActive(tab.id)}
        />
      ))}
    </div>
  );
}

function TabButton({
  tab,
  isActive,
  onClick,
}: {
  tab: DiscreteTabsItem;
  isActive: boolean;
  onClick: () => void;
}) {
  const Icon = tab.icon;

  return (
    <motion.button
      type="button"
      onClick={onClick}
      layout
      transition={{
        layout: {
          type: "spring",
          damping: 20,
          stiffness: 230,
          mass: 1.2,
          ease: [0.215, 0.61, 0.355, 1],
        },
      }}
      className="w-fit h-fit flex"
      style={{ willChange: "transform" }}
    >
      <motion.span
        layout
        transition={{
          layout: {
            type: "spring",
            damping: 20,
            stiffness: 230,
            mass: 1.2,
          },
        }}
        className={cn(
          "flex items-center gap-1.5 bg-secondary outline outline-2 outline-background overflow-hidden shadow-md transition-colors duration-75 ease-out p-3 cursor-pointer",
          isActive && "text-primary",
          isActive ? "px-4" : "px-3",
        )}
        style={{ borderRadius: 999 }}
      >
        <motion.span layout className="shrink-0" style={{ willChange: "transform" }}>
          <Icon size={20} />
        </motion.span>

        {isActive && (
          <motion.span
            initial={{ opacity: 0, filter: "blur(4px)" }}
            animate={{ opacity: 1, filter: "blur(0px)" }}
            transition={{ duration: 0.2, ease: [0.86, 0, 0.07, 1] }}
            className="text-sm font-medium whitespace-nowrap"
          >
            {tab.label}
          </motion.span>
        )}
      </motion.span>
    </motion.button>
  );
}
