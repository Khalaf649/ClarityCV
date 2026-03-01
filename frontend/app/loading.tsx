export default function Loading() {
  return (
    <main className="flex-1 flex items-center justify-center p-6">
      <div className="flex flex-col items-center gap-4">
        <div className="h-10 w-10 rounded-full border-2 border-primary border-t-transparent animate-spin" />
        <p className="text-sm font-mono text-muted-foreground animate-pulse">
          Loading…
        </p>
      </div>
    </main>
  );
}
