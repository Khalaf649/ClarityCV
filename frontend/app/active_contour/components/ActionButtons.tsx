"use client";

import { Button } from "@/components/ui/Button";

interface ActionButtonsProps {
  onApply: () => void;
  onReset: () => void;
  applyDisabled?: boolean;
  loading?: boolean;
}

export function ActionButtons({
  onApply,
  onReset,
  applyDisabled = false,
  loading = false,
}: ActionButtonsProps) {
  return (
    <div className="flex gap-2">
      <Button
        onClick={onApply}
        disabled={applyDisabled || loading}
        className="flex-1"
        size="sm"
      >
        Apply
      </Button>
      <Button
        onClick={onReset}
        variant="outline"
        className="flex-1"
        size="sm"
      >
        Reset
      </Button>
    </div>
  );
}
