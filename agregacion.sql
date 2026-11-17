-- ─────────────────────────────────────────────────────────────────
-- Agregación por período para la web.
-- Ejecutar en Supabase → SQL Editor (después de schema.sql).
-- Requiere PostgreSQL 14+ (date_bin); Supabase ya lo cumple.
--
--   periodo   ventana     intervalo   puntos aprox.
--   day       24 horas    10 minutos  144
--   week      7 días      1 hora      168
--   month     30 días     6 horas     120
--   year      365 días    1 día       365
--
-- Los intervalos se alinean a la medianoche de Buenos Aires (UTC-3).
-- ─────────────────────────────────────────────────────────────────

create or replace function public.lecturas_agrupadas(periodo text)
returns table (
  bucket    timestamptz,
  temp_prom numeric, temp_min numeric, temp_max numeric,
  hum_prom  numeric, hum_min  numeric, hum_max  numeric,
  n         bigint
)
language plpgsql
stable
set search_path = public
as $$
declare
  v_desde interval;
  v_paso  interval;
begin
  case periodo
    when 'day'   then v_desde := interval '24 hours'; v_paso := interval '10 minutes';
    when 'week'  then v_desde := interval '7 days';   v_paso := interval '1 hour';
    when 'month' then v_desde := interval '30 days';  v_paso := interval '6 hours';
    when 'year'  then v_desde := interval '365 days'; v_paso := interval '1 day';
    else raise exception 'periodo invalido: % (usar day, week, month o year)', periodo;
  end case;

  return query
    select
      date_bin(v_paso, l.created_at, timestamptz '2000-01-01 03:00:00+00') as bucket,
      round(avg(l.temperatura), 1), round(min(l.temperatura), 1), round(max(l.temperatura), 1),
      round(avg(l.humedad), 1),     round(min(l.humedad), 1),     round(max(l.humedad), 1),
      count(*)
    from lecturas_sensor l
    where l.created_at >= now() - v_desde
    group by 1
    order by 1;
end;
$$;

grant execute on function public.lecturas_agrupadas(text) to anon, authenticated;
