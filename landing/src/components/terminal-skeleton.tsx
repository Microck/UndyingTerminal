"use client";

export function TerminalSkeleton() {
  return (
    <div className="w-full">
      <div className="terminal-window grid grid-cols-[minmax(100px,140px)_1fr] sm:grid-cols-[minmax(120px,160px)_1fr] w-full animate-pulse">
        {/* Status rail skeleton */}
        <div className="border-r border-white/5 bg-[rgba(255,255,255,0.02)] p-2 sm:p-3">
          <div className="mb-3 h-3 w-12 bg-white/10 rounded" />
          <div className="space-y-2">
            <div className="flex items-center gap-2">
              <div className="w-2 h-2 rounded-full bg-white/10" />
              <div className="h-3 w-16 bg-white/10 rounded" />
            </div>
            <div className="mt-3 space-y-1">
              <div className="h-3 w-20 bg-white/10 rounded" />
              <div className="h-3 w-16 bg-white/10 rounded" />
              <div className="h-3 w-24 bg-white/10 rounded" />
            </div>
          </div>
        </div>

        {/* Terminal body skeleton */}
        <div className="min-w-0">
          <div className="terminal-header">
            <div className="h-3 w-20 bg-white/10 rounded" />
            <div className="flex-1" />
            <div className="h-3 w-24 bg-white/10 rounded" />
          </div>
          <div className="terminal-body p-5 space-y-3">
            <div className="h-4 w-3/4 bg-white/10 rounded" />
            <div className="h-4 w-1/2 bg-white/10 rounded" />
            <div className="h-4 w-2/3 bg-white/10 rounded" />
            <div className="h-4 w-4/5 bg-white/10 rounded" />
          </div>
        </div>
      </div>

      {/* CTA buttons skeleton */}
      <div className="flex flex-wrap gap-4 mt-8">
        <div className="h-10 w-40 bg-white/10 rounded" />
        <div className="h-10 w-32 bg-white/10 rounded" />
      </div>

      {/* Version text skeleton */}
      <div className="h-4 w-48 bg-white/10 rounded mt-5" />
    </div>
  );
}
