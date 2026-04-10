import Link from "next/link";
import { ChevronRight } from 'lucide-react';
import { appGroups } from '@/lib/appGroups';

export default function HomePage() {
  return (
    <div className="p-6 md:p-8 max-w-7xl mx-auto w-full">
      {/* Hero Section */}
      <div className="text-center mb-12 mt-4">
        <h1 className="text-4xl md:text-5xl font-bold text-white mb-4 tracking-tight">
          <span className="text-cyan-400">Clairty</span>Cv
        </h1>
        <p className="text-slate-400 max-w-2xl mx-auto text-lg leading-relaxed">
          An interactive computer-vision laboratory. Upload an image once and
          explore spatial filtering, histogram analysis, and frequency-domain
          operations — all from your browser.
        </p>
      </div>

      {/* Categorized Route Cards */}
      {appGroups.map((group, idx) => (
        <div key={idx} className="mb-12">
          <h2 className="text-xl font-semibold text-white mb-6 flex items-center gap-2 border-b border-slate-800 pb-2">
            {group.category}
          </h2>
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            {group.tools.map(tool => (
              <Link 
                href={tool.href} 
                key={tool.href} 
                className="bg-slate-900 border border-slate-800 hover:border-cyan-500/50 rounded-xl p-6 transition-all duration-300 hover:shadow-[0_0_15px_rgba(34,211,238,0.1)] group flex flex-col h-full outline-none focus:ring-2 focus:ring-cyan-500/50"
              >
                <div className="w-10 h-10 rounded-lg bg-slate-800 flex items-center justify-center text-cyan-400 mb-4 group-hover:scale-110 transition-transform">
                  {tool.icon}
                </div>
                <h3 className="text-lg font-semibold font-mono text-white mb-2 group-hover:text-cyan-400 transition-colors">
                  {tool.title}
                </h3>
                <p className="text-slate-400 text-sm leading-relaxed flex-1">
                  {tool.description}
                </p>
                <div className="mt-4 flex items-center text-cyan-400 text-sm font-medium opacity-0 group-hover:opacity-100 transition-opacity transform translate-x-[-10px] group-hover:translate-x-0 duration-300">
                  Launch Tool <ChevronRight size={16} className="ml-1" />
                </div>
              </Link>
            ))}
          </div>
        </div>
      ))}
    </div>
  );
}