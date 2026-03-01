interface ControlPanelProps {
  title: string;
  children: React.ReactNode;
  className?: string;
}

export function ControlPanel({
  title,
  children,
  className = "",
}: ControlPanelProps) {
  return (
    <div className={`control-panel ${className}`}>
      <h3 className="text-sm font-mono text-primary uppercase tracking-wider">
        {title}
      </h3>
      {children}
    </div>
  );
}
